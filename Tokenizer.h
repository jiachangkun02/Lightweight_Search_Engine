#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>


namespace jia
{
    class Tokenizer
    {
    public:
        
        Tokenizer();
        // 详细说明：构造函数，初始化分词器并加载停用词列表
        // 参数：无
        // 返回值：无

        
        std::vector<std::string> tokenize(const std::string& text) const;
        // 详细说明：对文本内容进行分词处理，包括停用词过滤
        // 参数：
        //   text - 要分词的原始文本
        // 返回值：分词后的词汇列表，已过滤停用词

        std::vector<std::string> tokenize_raw_query(const std::string& text) const;
        // 详细说明：对查询字符串进行分词处理，保留操作符和短语结构
        // 参数：
        //   text - 查询字符串
        // 返回值：分词后的查询令牌，保留布尔操作符和引号

    private:
        std::unordered_set<std::string> stop_words_;
        // 详细说明：停用词集合，包含常见但无实际意义的词汇
        // 用于在分词时过滤这些词汇，提高索引质量


        std::vector<std::string> perform_tokenization(const std::string& text, bool remove_stopwords)const;
        // 详细说明：执行实际的分词操作，支持停用词过滤控制
        // 参数：
        //   text - 要分词的文本
        //   remove_stopwords - 是否移除停用词
        // 返回值：分词后的词汇列表
    };
}

