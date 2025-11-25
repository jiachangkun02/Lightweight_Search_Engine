#include "Tokenizer.h"
#include "Utils.h"
#include <string>
#include <algorithm>
#include <cctype>
#include <sstream>

using namespace jia;

Tokenizer::Tokenizer()
{
    //停用词列表 说人话 就是将一些高频出现但不重要的词添加进去 不用为这些词建立索引 省事
    stop_words_={
        "a","the","is","as","at","an","this","what","that","will","are","were","was"
    };
}


std::vector<std::string> Tokenizer::tokenize(const std::string& text) const
{
    // 核心功能：将普通文本拆分为适合索引的单词序列。
    // 原理与流程：
    // 1) 调用 perform_tokenization，统一转小写并可选移除停用词；
    // 2) 依赖 isalnum 判定单词边界，过滤标点与特殊符号；
    // 3) 返回的 token 全部为小写，便于与索引匹配。
    // std::vector<std::string> result;
    // std::string current_token;
    //
    // for (char ch : text)
    // {
    //     if (std::isalpha(ch) || std::isdigit(ch))
    //     {
    //         //收集字母和数字
    //         current_token += std::tolower(ch);
    //     }
    //     else if (!current_token.empty())
    //     {
    //         //遇到其他字符时，保存当前token
    //         result.push_back(current_token);
    //         current_token.clear();
    //     }
    // }
    //
    // //处理最后一个token
    // if (!current_token.empty())
    // {
    //     result.push_back(current_token);
    // }
    //
    // return result;
    //
    // //return perform_tokenization(text,true);
    //  统一使用带停用词过滤的分词函数，保证索引与查询一致
    return perform_tokenization(text, true);
}



std::vector<std::string> Tokenizer::tokenize_raw_query(const std::string& text) const
{
    // 核心功能：将原始查询拆分为适合布尔解析的 token 序列。
    // 原理与流程：
    // 1) 保留用户原始大小写，便于识别 AND/OR/NOT、括号等逻辑符号；
    // 2) 支持用引号括起的短语，拆成带前缀 "__PHRASE__" 的单个 token；
    // 3) 括号单独成 token，方便递归下降解析；
    // 4) 其余按空白分词，保留原样交由上层判定大小写与逻辑。
    // std::vector<std::string> tokens;
    // std::istringstream iss(text);
    // std::string token;
    //
    // //简单按空格分词，保持原始形式
    // while (iss >> token)
    // {
    //     tokens.push_back(token);
    // }
    //
    // return tokens;
    //  解析引号内的短语，并保持原样
    std::vector<std::string> tokens;
    std::string current;
    bool in_quote = false;
    for (size_t i = 0; i < text.size(); ++i)
    {
        char c = text[i];
        if (c == '\"')
        {
            if (in_quote)
            {
                // 结束短语
                tokens.push_back("__PHRASE__" + current);
                current.clear();
                in_quote = false;
            }
            else
            {
                // 开始短语
                if (!current.empty())
                {
                    tokens.push_back(current);
                    current.clear();
                }
                in_quote = true;
            }
        }
        else if ((c == '(' || c == ')') && !in_quote)
        {
            if (!current.empty())
            {
                tokens.push_back(current);
                current.clear();
            }
            tokens.push_back(std::string(1, c));
        }
        else if (std::isspace(static_cast<unsigned char>(c)) && !in_quote)
        {
            if (!current.empty())
            {
                tokens.push_back(current);
                current.clear();
            }
        }
        else
        {
            current.push_back(c);
        }
    }
    if (!current.empty())
    {
        if (in_quote)
        {
            tokens.push_back("__PHRASE__" + current);
        }
        else
        {
            tokens.push_back(current);
        }
    }
    return tokens;
}



std::vector<std::string> Tokenizer::perform_tokenization(const std::string& text, bool remove_stopwords) const
{
    // 核心功能：基础分词实现，负责：
    // 1) 逐字符扫描，遇到字母/数字则累积成 token，遇到非字母数字则提交当前 token；
    // 2) 将 token 统一转为小写；
    // 3) 可选移除停用词（remove_stopwords=true 时过滤高频无信息词）；
    // 4) 返回适合索引/查询的词序列。
    std::vector<std::string> tokens;
    std::string current_token;

    auto process_token = [&](const std::string& token_to_process)
        {
            if (token_to_process.empty())
            {
                return;
            }
            std::string lower_token = jia::to_lower(token_to_process);

            if (remove_stopwords)
            {
                if (stop_words_.find(lower_token)==stop_words_.end())
                {
                    tokens.push_back(lower_token);
                }
            }
            else
            {
                tokens.push_back(lower_token);
            }
        
        };

    for (unsigned char c : text)
    {
        if (std::isalnum(c))
        {
            current_token += static_cast<char>(c);
        }
        else
        {
            process_token(current_token);

            current_token.clear();
        }
        
    }
    process_token(current_token);



    return tokens;
    
}

