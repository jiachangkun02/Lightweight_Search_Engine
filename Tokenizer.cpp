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
    //对普通文本进行分词
    //1. 按空格分割
    //2. 转换为小写
    //3. 去除标点符号
    
    std::vector<std::string> result;
    std::string current_token;

    for (char ch : text)
    {
        if (std::isalpha(ch) || std::isdigit(ch))
        {
            //收集字母和数字
            current_token += std::tolower(ch);
        }
        else if (!current_token.empty())
        {
            //遇到其他字符时，保存当前token
            result.push_back(current_token);
            current_token.clear();
        }
    }

    //处理最后一个token
    if (!current_token.empty())
    {
        result.push_back(current_token);
    }

    return result;

    //return perform_tokenization(text,true);
}



std::vector<std::string> Tokenizer::tokenize_raw_query(const std::string& text) const
{
    //对查询字符串进行分词
    //特殊处理：
    //1. 保留AND、OR、NOT等关键词
    //2. 支持引号内的短语
    //3. 保持原始大小写（便于识别关键词）
    
    std::vector<std::string> tokens;
    std::istringstream iss(text);
    std::string token;

    //简单按空格分词，保持原始形式
    while (iss >> token)
    {
        tokens.push_back(token);
    }

    return tokens;
}



std::vector<std::string> Tokenizer::perform_tokenization(const std::string& text, bool remove_stopwords) const
{
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

    for (const auto& c : text)
    {
        if (std::isalnum(static_cast<char>(c)))
        {
            current_token += c;
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
