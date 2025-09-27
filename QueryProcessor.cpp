#include "QueryProcessor.h"
#include "Utils.h"
#include <algorithm>
#include <execution>
#include <sstream>

using namespace jia;
using namespace std;

// 构造函数：保存对倒排索引的引用，后续查询都基于这个索引
QueryProcessor::QueryProcessor(InvertedIndex& index)
    :index_(index)
{
    
}

// 对外查询入口：把传入的查询字符串交给布尔查询评估器并返回结果
std::vector<ScoredDoc> QueryProcessor::process(const std::string& term)
{
    return evaluate_boolean_query(term);
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
        std::string upper_token = to_lower(token);
        std::transform(upper_token.begin(), upper_token.end(), upper_token.begin(), ::toupper);

        if (upper_token=="AND")
        {
            continue;
        }
        else if (upper_token == "NOT")
        {
            expect_NOT = true;
        }
        else
        {
            if (expect_NOT)
            {
                negative_terms.push_back(token);
                expect_NOT = false;
            }
            else
            {
                positive_terms.push_back(token);
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
std::vector<ScoredDoc> QueryProcessor::evaluate_boolean_query(const std::string& query_string)
{
    std::vector<std::string> raw_tokens = query_tokenizer_.tokenize_raw_query(query_string);


    //std::istringstream iss(tokens);
    //std::string token;
    std::vector<std::vector<std::string>> OR_part;
    std::vector<std::string> current_part;


    /*while (iss>>token)
    {
        std::string upper_token=to_lower(token);
        std::transform(upper_token.begin(), upper_token.end(), upper_token.begin(), ::toupper);


        if (upper_token=="AND")
        {
            continue;
        }
        else if (upper_token == "NOT")
        {
            expect_NOT = true;
        }
        else
        {
            std::string ready_token=to_lower(upper_token);
            if (expect_NOT==true)
            {
                negative_terms.push_back(ready_token);
                expect_NOT = false;
            }
            else
            {
                positive_terms.push_back(ready_token);
            }
        }

    }*/


    
    for (const auto & token : raw_tokens)
    {
        if (to_lower(token)=="or")
        {
            if (!current_part.empty())
            {
                OR_part.push_back(current_part);
                current_part.clear();
            }
        }
        else
        {
            current_part.push_back(token);
        }
    }

    if (!current_part.empty())
    {
        OR_part.push_back(current_part);
    }

    if (OR_part.empty())
    {
        return {};
    }
    
    std::unordered_map<DocId, double> merged_Scores;
    for (const auto & part : OR_part)
    {
        auto result=evaluate_AND_NOT_query(part);
        for (const auto & doc : result)
        {
            // 合并策略：同一 doc 取最大的分数（OR 意味任一子句命中即可）
            if (merged_Scores.find(doc.doc_id)==merged_Scores.end()||doc.score>merged_Scores[doc.doc_id])
            {
                merged_Scores[doc.doc_id] = doc.score;
            }
        }
    }

    std::vector<ScoredDoc> final_result;
    for (const auto & pair : merged_Scores)
    {
        final_result.push_back({ pair.first,pair.second });
    }


    // 并行排序，加速大结果集的排序操作
    std::sort(std::execution::par,final_result.begin(), final_result.end(), std::greater<ScoredDoc>());
    
    return final_result;
    
}








