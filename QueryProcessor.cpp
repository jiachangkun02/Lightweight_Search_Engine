#include "QueryProcessor.h"
#include "Utils.h"
#include <algorithm>
#include <execution>
#include <sstream>

using namespace jia;
using namespace std;

// ���캯��������Ե������������ã�������ѯ�������������
QueryProcessor::QueryProcessor(InvertedIndex& index)
    :index_(index)
{
    
}

// �����ѯ��ڣ��Ѵ���Ĳ�ѯ�ַ�������������ѯ�����������ؽ��
std::vector<ScoredDoc> QueryProcessor::process(const std::string& term)
{
    return evaluate_boolean_query(term);
}

// ʹ�� TF-IDF �Ը����Ĵʼ��Ͻ��д�ֲ�����������
// �����߼���
//  1) ����ÿ���ʵ� idf = log(1 + N / df)
//  2) ��ÿ�����ֵ��ĵ����ۼ� tf * idf
//  3) ���ĵ����ȹ�һ�������������ĵ����ȣ�
//  4) ���������򷵻�
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

        // �ص��������
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
        // ���ĵ��������򵥵Ĺ�һ�������ⳤ�ĵ���Ȼ�÷ֹ���
        pair.second /= std::max(1, index_.get_docINF(pair.first).length);
        result.push_back({ pair.first,pair.second });
    }
    // �������У��÷ָߵ���ǰ��
    std::sort(result.begin(), result.end(), [](const ScoredDoc& a, const ScoredDoc& b)
    {
        return a.score > b.score;
    });
    
    return result;
}

// ��������: term1 AND term2 AND NOT term3 ... ���Ӳ�ѯ
// ���򣨼򵥰棩��
//  - �� NOT ��Ĵ��ռ��� negative_terms
//  - �������Ϊ positive_terms
//  - �� positive_terms �� TF-IDF ����õ���ѡ��
//  - �Ӻ�ѡ�����Ƴ��� negative_terms ���ֵ��ĵ�
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

    // �ȶ�������� TF-IDF ����
    auto ranked_result = rank_TF_IDF(positive_terms);

    if (negative_terms.empty())
    {
        return ranked_result;
    }

    // �� NOT ����Ĵʹ��˵������������Ľ��
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

// ������ѯ��ڣ�֧�� OR, AND, NOT �ļ����
// ʵ��˼·��
//  - ���� tokenizer ��ԭʼ��ѯ�зֳ� token
//  - ���� OR ����ѯ�ֳ������Ӿ䣨ÿ���Ӿ��ڲ��� AND/NOT ����
//  - ��ÿ���Ӿ���� evaluate_AND_NOT_query ��ô���б�
//  - �ϲ����Ӿ�Ľ����ͬһ�ĵ�ȡ��߷�
//  - ��������򷵻�
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
            // �ϲ����ԣ�ͬһ doc ȡ���ķ�����OR ��ζ��һ�Ӿ����м��ɣ�
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


    // �������򣬼��ٴ��������������
    std::sort(std::execution::par,final_result.begin(), final_result.end(), std::greater<ScoredDoc>());
    
    return final_result;
    
}








