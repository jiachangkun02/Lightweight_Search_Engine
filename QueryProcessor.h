#pragma once
#include "IndexData.h"
#include "InvertedIndex.h"
using namespace std;

namespace jia
{
    class Tokenizer;
    
    
    class QueryProcessor
    {
    public:
        explicit QueryProcessor(InvertedIndex& index);


        std::vector<ScoredDoc> process(const std::string& terms);
        
    private:

        //计算某个词的排名结果依据TF—IDF算法
        std::vector<ScoredDoc> rank_TF_IDF(const std::vector<std::string>& terms) const;

        std::vector<ScoredDoc> evaluate_AND_NOT_query(const std::vector<std::string>& tokens);


        std::vector<ScoredDoc> evaluate_boolean_query(const std::string& query_string);
        
        InvertedIndex& index_;


        Tokenizer query_tokenizer_;
        
    };
}

