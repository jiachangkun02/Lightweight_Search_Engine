#include "QueryProcessor.h"
#include "Utils.h"
#include <algorithm>
#include <execution>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <cmath>

using namespace jia;
using namespace std;

// 构造函数：保存对倒排索引的引用，提供查询入口的上下文。
QueryProcessor::QueryProcessor(InvertedIndex& index)
    :index_(index)
{
    
}

// 对外查询入口：接收查询字符串与查询参数，交给布尔解析器并返回最终排序结果；内部会融合默认 BM25 选项。
std::vector<ScoredDoc> QueryProcessor::process(const std::string& term, const QueryOptions& options)
{
    // return evaluate_boolean_query(term);
    QueryOptions merged = options;
    merged.use_bm25 = merged.use_bm25 || this->default_use_bm25_;
    return evaluate_boolean_query(term, merged);
}

// 使用 TF-IDF 对给定的词集合进行打分并返回排序结果
// 评分逻辑：
//  1) 计算每个词的 idf = log(1 + N / df)
//  2) 对每个出现的文档，累加 tf * idf
//  3) 用文档长度归一化分数（除以文档长度）
//  4) 按分数降序返回
std::vector<ScoredDoc> QueryProcessor::rank_TF_IDF(const std::vector<std::string>& terms) const
{
    int N = index_.get_doc_count();
    if (N == 0)
    {
        return {};
    }

    std::unordered_map<DocId, double> docIdWITHscores;

    for (const auto& term : terms)
    {
        int df=index_.get_term_frequency_inDoc(term);
        if (df==0)
        {
            continue;
        }

        // 重点计算评分
        double idf=std::log(1.0+static_cast<double>(N)/df);

        const auto* posting_entries = index_.get_posting_entries(term);

        if (!posting_entries)
        {
            continue;
        }
          
        for (const auto& entry : *posting_entries)
        {
            double tf = static_cast<double>(entry.frequency);


            docIdWITHscores[entry.doc_id_] += tf * idf;
        }

    }
    
    std::vector<ScoredDoc> result;
    for (auto& pair : docIdWITHscores)
    {
        // 用文档长度做简单的归一化，避免长文档天然得分过高
        pair.second /= std::max(1, index_.get_docINF(pair.first).length);
        result.push_back({ pair.first,pair.second });
    }
    // 降序排列，得分高的排前面
    std::sort(result.begin(), result.end(), [](const ScoredDoc& a, const ScoredDoc& b)
    {
        return a.score > b.score;
    });
    
    return result;
}

//  BM25 排序实现
std::vector<ScoredDoc> QueryProcessor::rank_BM25(const std::vector<std::string>& terms) const
{
    int N = index_.get_doc_count();
    if (N == 0)
    {
        return {};
    }
    double avg_len = index_.get_avg_doc_length();
    if (avg_len <= 0.0)
    {
        avg_len = 1.0;
    }
    const double k1 = 1.5;
    const double b = 0.75;
    std::unordered_map<DocId, double> scores;
    for (auto term : terms)
    {
        term = jia::to_lower(term);
        int df = index_.get_term_frequency_inDoc(term);
        if (df == 0)
        {
            continue;
        }
        double idf = std::log((static_cast<double>(N) - df + 0.5) / (df + 0.5) + 1.0);
        const auto* postings = index_.get_posting_entries(term);
        if (!postings)
        {
            continue;
        }
        for (const auto& entry : *postings)
        {
            double tf = static_cast<double>(entry.frequency);
            double dl = std::max(1, index_.get_docINF(entry.doc_id_).length);
            double denom = tf + k1 * (1 - b + b * (dl / avg_len));
            double bm25 = idf * ((tf * (k1 + 1)) / denom);
            scores[entry.doc_id_] += bm25;
        }
    }
    std::vector<ScoredDoc> result;
    for (const auto& p : scores)
    {
        result.push_back({ p.first, p.second });
    }
    std::sort(result.begin(), result.end(), [](const ScoredDoc& a, const ScoredDoc& b)
    {
        return a.score > b.score;
    });
    return result;
}

//  统一入口：根据开关选择 TF-IDF 或 BM25
std::vector<ScoredDoc> QueryProcessor::rank_terms(const std::vector<std::string>& terms, bool use_bm25) const
{
    if (use_bm25)
    {
        return rank_BM25(terms);
    }
    return rank_TF_IDF(terms);
}

//  短语查询：要求连续位置匹配
std::vector<ScoredDoc> QueryProcessor::rank_phrase(const std::vector<std::string>& phrase_terms) const
{
    if (phrase_terms.empty())
    {
        return {};
    }
    const auto* first_postings = index_.get_posting_entries(phrase_terms[0]);
    if (!first_postings)
    {
        return {};
    }
    std::unordered_map<DocId, double> scores;
    for (const auto& first_entry : *first_postings)
    {
        bool all_found = true;
        std::vector<const PostingEntry*> other_entries;
        for (size_t i = 1; i < phrase_terms.size(); ++i)
        {
            const auto* postings = index_.get_posting_entries(phrase_terms[i]);
            const PostingEntry* target = nullptr;
            if (postings)
            {
                for (const auto& e : *postings)
                {
                    if (e.doc_id_ == first_entry.doc_id_)
                    {
                        target = &e;
                        break;
                    }
                }
            }
            if (!target)
            {
                all_found = false;
                break;
            }
            other_entries.push_back(target);
        }
        if (!all_found)
        {
            continue;
        }
        int phrase_hits = 0;
        for (int pos : first_entry.position)
        {
            bool matched = true;
            for (size_t i = 0; i < other_entries.size(); ++i)
            {
                int desired = pos + static_cast<int>(i) + 1;
                if (std::find(other_entries[i]->position.begin(), other_entries[i]->position.end(), desired) == other_entries[i]->position.end())
                {
                    matched = false;
                    break;
                }
            }
            if (matched)
            {
                phrase_hits++;
            }
        }
        if (phrase_hits > 0)
        {
            scores[first_entry.doc_id_] = static_cast<double>(phrase_hits);
        }
    }
    std::vector<ScoredDoc> result;
    for (const auto& p : scores)
    {
        result.push_back({ p.first, p.second });
    }
    std::sort(result.begin(), result.end(), [](const ScoredDoc& a, const ScoredDoc& b)
    {
        return a.score > b.score;
    });
    return result;
}

// 评估形如: term1 AND term2 AND NOT term3 ... 的子查询
// 规则（简单版）：
//  - 将 NOT 后的词收集到 negative_terms
//  - 其余词作为 positive_terms
//  - 对 positive_terms 用 TF-IDF 排序得到候选集
//  - 从候选集中移除在 negative_terms 出现的文档
std::vector<ScoredDoc> QueryProcessor::evaluate_AND_NOT_query(const std::vector<std::string>& tokens)
{
    std::vector<std::string> positive_terms;
    std::vector<std::string> negative_terms;
    bool expect_NOT = false;


    for (const auto& token : tokens)
    {
        // std::string upper_token = to_lower(token);
        // std::transform(upper_token.begin(), upper_token.end(), upper_token.begin(), ::toupper);
        //
        // if (upper_token=="AND")
        // {
        //     continue;
        // }
        // else if (upper_token == "NOT")
        // {
        //     expect_NOT = true;
        // }
        // else
        // {
        //     if (expect_NOT)
        //     {
        //         negative_terms.push_back(token);
        //         expect_NOT = false;
        //     }
        //     else
        //     {
        //         positive_terms.push_back(token);
        //     }
        // }
        //  统一对操作符大小写不敏感，并对实际检索词统一小写以匹配索引
        std::string normalized_token = jia::to_lower(token);
        if (normalized_token == "and")
        {
            continue;
        }
        else if (normalized_token == "not")
        {
            expect_NOT = true;
        }
        else
        {
            if (expect_NOT)
            {
                negative_terms.push_back(normalized_token);
                expect_NOT = false;
            }
            else
            {
                positive_terms.push_back(normalized_token);
            }
        }
    }

    if (positive_terms.empty())
    {
        return {};
    }

    // 先对正项进行 TF-IDF 排序
    auto ranked_result = rank_TF_IDF(positive_terms);

    if (negative_terms.empty())
    {
        return ranked_result;
    }

    // 用 NOT 后面的词过滤掉不符合条件的结果
    std::unordered_set<DocId> excluded_doc;
    for (const auto& term : negative_terms)
    {
        const auto* postings=index_.get_posting_entries(term);

        if (postings)
        {
            for (const auto& entry : *postings)
            {
                excluded_doc.insert(entry.doc_id_);
            }
        }
    }


    std::vector<ScoredDoc> final_doc;

    for (const auto & scored_doc : ranked_result)
    {
        if (excluded_doc.find(scored_doc.doc_id)==excluded_doc.end())
        {
            final_doc.push_back(scored_doc);
        }
    }

    return final_doc;
    
}

// 布尔查询入口，支持 OR, AND, NOT 的简单组合
// 实现思路：
//  - 先用 tokenizer 把原始查询切分成 token
//  - 根据 OR 将查询分成若干子句（每个子句内部按 AND/NOT 处理）
//  - 对每个子句调用 evaluate_AND_NOT_query 获得打分列表
//  - 合并各子句的结果：同一文档取最高分
//  - 最后并行排序返回
std::vector<ScoredDoc> QueryProcessor::evaluate_boolean_query(const std::string& query_string, const QueryOptions& options)
{
    // std::vector<std::string> raw_tokens = query_tokenizer_.tokenize_raw_query(query_string);
    //
    // //std::istringstream iss(tokens);
    // //std::string token;
    // std::vector<std::vector<std::string>> OR_part;
    // std::vector<std::string> current_part;
    //
    //
    // /*while (iss>>token)
    // {
    //     std::string upper_token=to_lower(token);
    //     std::transform(upper_token.begin(), upper_token.end(), upper_token.begin(), ::toupper);
    //
    //
    //     if (upper_token=="AND")
    //     {
    //         continue;
    //     }
    //     else if (upper_token == "NOT")
    //     {
    //         expect_NOT = true;
    //     }
    //     else
    //     {
    //         std::string ready_token=to_lower(upper_token);
    //         if (expect_NOT==true)
    //         {
    //             negative_terms.push_back(ready_token);
    //             expect_NOT = false;
    //         }
    //         else
    //         {
    //             positive_terms.push_back(ready_token);
    //         }
    //     }
    //
    // }*/
    //
    //
    // for (const auto & token : raw_tokens)
    // {
    //     if (to_lower(token)=="or")
    //     {
    //         if (!current_part.empty())
    //         {
    //             OR_part.push_back(current_part);
    //             current_part.clear();
    //         }
    //     }
    //     else
    //     {
    //         current_part.push_back(token);
    //     }
    // }
    //
    // if (!current_part.empty())
    // {
    //     OR_part.push_back(current_part);
    // }
    //
    // if (OR_part.empty())
    // {
    //     return {};
    // }
    //
    // std::unordered_map<DocId, double> merged_Scores;
    // for (const auto & part : OR_part)
    // {
    //     auto result=evaluate_AND_NOT_query(part);
    //     for (const auto & doc : result)
    //     {
    //         // 合并策略：同一 doc 取最大的分数（OR 意味任一子句命中即可）
    //         if (merged_Scores.find(doc.doc_id)==merged_Scores.end()||doc.score>merged_Scores[doc.doc_id])
    //         {
    //             merged_Scores[doc.doc_id] = doc.score;
    //         }
    //     }
    // }
    //
    // std::vector<ScoredDoc> final_result;
    // for (const auto & pair : merged_Scores)
    // {
    //     final_result.push_back({ pair.first,pair.second });
    // }
    //
    //
    // // 并行排序，加速大结果集的排序操作
    // std::sort(std::execution::par,final_result.begin(), final_result.end(), std::greater<ScoredDoc>());
    //
    // return final_result;
    //  新版：支持括号、NOT 优先、短语、BM25 及分页
    std::vector<std::string> tokens = query_tokenizer_.tokenize_raw_query(query_string);
    size_t pos = 0;
    auto result = evaluate_expression(tokens, pos, options);
    std::sort(result.begin(), result.end(), std::greater<ScoredDoc>());
    //  Apply offset and top_k
    if (options.offset >= result.size())
    {
        return {};
    }
    size_t end = std::min(result.size(), options.offset + options.top_k);
    return std::vector<ScoredDoc>(result.begin() + options.offset, result.begin() + end);
}

//  递归下降解析：expression -> term { OR term }
std::vector<ScoredDoc> QueryProcessor::evaluate_expression(const std::vector<std::string>& tokens, size_t& pos, const QueryOptions& options)
{
    auto left_map = scored_to_map(evaluate_term(tokens, pos, options));
    while (pos < tokens.size())
    {
        std::string op = jia::to_lower(tokens[pos]);
        if (op != "or")
        {
            break;
        }
        ++pos;
        auto right_map = scored_to_map(evaluate_term(tokens, pos, options));
        left_map = merge_or(left_map, right_map);
    }
    return vector_to_scored(left_map);
}

//  term -> factor { AND factor }，负责处理中间优先级的 AND 以及前置 NOT。，支持 NOT
std::vector<ScoredDoc> QueryProcessor::evaluate_term(const std::vector<std::string>& tokens, size_t& pos, const QueryOptions& options)
{
    std::unordered_map<DocId, double> current;
    bool initialized = false;
    while (pos < tokens.size())
    {
        std::string token_lower = jia::to_lower(tokens[pos]);
        if (token_lower == "or" || tokens[pos] == ")")
        {
            break;
        }
        if (token_lower == "and")
        {
            ++pos;
            continue;
        }
        bool is_not = false;
        if (token_lower == "not")
        {
            is_not = true;
            ++pos;
        }
        auto factor_vec = evaluate_factor(tokens, pos, options);
        auto factor_map = scored_to_map(factor_vec);
        if (!initialized)
        {
            if (is_not)
            {
                current = merge_not(get_all_docs_universe(), factor_map);
            }
            else
            {
                current = factor_map;
            }
            initialized = true;
        }
        else
        {
            if (is_not)
            {
                current = merge_not(current, factor_map);
            }
            else
            {
                current = merge_and(current, factor_map);
            }
        }
    }
    return vector_to_scored(current);
}

//  factor -> (expression) | term | phrase：基础单元，支持括号分组与短语查询。
std::vector<ScoredDoc> QueryProcessor::evaluate_factor(const std::vector<std::string>& tokens, size_t& pos, const QueryOptions& options)
{
    if (pos >= tokens.size())
    {
        return {};
    }
    if (tokens[pos] == "(")
    {
        ++pos;
        auto res = evaluate_expression(tokens, pos, options);
        if (pos < tokens.size() && tokens[pos] == ")")
        {
            ++pos;
        }
        return res;
    }
    std::vector<ScoredDoc> result;
    std::string token_value = tokens[pos];
    if (is_phrase_token(token_value))
    {
        auto phrase_terms = extract_phrase_terms(token_value);
        result = rank_phrase(phrase_terms);
    }
    else
    {
        result = rank_terms({ jia::to_lower(token_value) }, options.use_bm25);
    }
    ++pos;
    return result;
}

//  map <-> vector 转换
std::vector<ScoredDoc> QueryProcessor::vector_to_scored(const std::unordered_map<DocId, double>& m) const
{
    std::vector<ScoredDoc> v;
    v.reserve(m.size());
    for (const auto& p : m)
    {
        v.push_back({ p.first, p.second });
    }
    return v;
}

std::unordered_map<DocId, double> QueryProcessor::scored_to_map(const std::vector<ScoredDoc>& v) const
{
    std::unordered_map<DocId, double> m;
    for (const auto& s : v)
    {
        m[s.doc_id] = s.score;
    }
    return m;
}

//  OR 合并：相同文档取最大分
std::unordered_map<DocId, double> QueryProcessor::merge_or(const std::unordered_map<DocId, double>& a, const std::unordered_map<DocId, double>& b) const
{
    std::unordered_map<DocId, double> res = a;
    for (const auto& p : b)
    {
        auto it = res.find(p.first);
        if (it == res.end() || p.second > it->second)
        {
            res[p.first] = p.second;
        }
    }
    return res;
}

//  AND 合并：交集并累加得分
std::unordered_map<DocId, double> QueryProcessor::merge_and(const std::unordered_map<DocId, double>& a, const std::unordered_map<DocId, double>& b) const
{
    std::unordered_map<DocId, double> res;
    for (const auto& p : a)
    {
        auto it = b.find(p.first);
        if (it != b.end())
        {
            res[p.first] = p.second + it->second;
        }
    }
    return res;
}

//  NOT 合并：保留 a 中不在 b 的文档
std::unordered_map<DocId, double> QueryProcessor::merge_not(const std::unordered_map<DocId, double>& a, const std::unordered_map<DocId, double>& b) const
{
    std::unordered_map<DocId, double> res;
    for (const auto& p : a)
    {
        if (b.find(p.first) == b.end())
        {
            res[p.first] = p.second;
        }
    }
    return res;
}

//  获取全集 doc 作为 NOT 的起点
std::unordered_map<DocId, double> QueryProcessor::get_all_docs_universe() const
{
    std::unordered_map<DocId, double> res;
    for (auto id : index_.get_all_doc_ids())
    {
        res[id] = 0.0;
    }
    return res;
}

//  短语 token 判定
bool QueryProcessor::is_phrase_token(const std::string& token) const
{
    return token.rfind("__PHRASE__", 0) == 0;
}

//  拆解短语为词项
std::vector<std::string> QueryProcessor::extract_phrase_terms(const std::string& phrase_token) const
{
    std::string body = phrase_token.substr(std::string("__PHRASE__").size());
    // 复用查询分词器，确保与索引一致
    return query_tokenizer_.tokenize(body);
}










