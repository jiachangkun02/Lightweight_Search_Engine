#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>


namespace jia
{
    class Tokenizer
    {
        public
        :
        //停用词列表 说人话 就是将一些高频出现但不重要的词添加进去 不用为这些词建立索引 省事
        Tokenizer();

        //将文本切割
        std::vector<std::string> tokenize(const std::string& text) const;

        std::vector<std::string> tokenize_raw_query(const std::string& text) const;

        private
        :
        std::unordered_set<std::string> stop_words_;


        std::vector<std::string> perform_tokenization(const std::string& text, bool remove_stopwords)const;
    };
}

