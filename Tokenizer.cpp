#include "Tokenizer.h"
#include "Utils.h"
#include <string>
#include <algorithm>
#include <cctype>
#include <sstream>

using namespace jia;

Tokenizer::Tokenizer()
{
    //ͣ�ô��б� ˵�˻� ���ǽ�һЩ��Ƶ���ֵ�����Ҫ�Ĵ���ӽ�ȥ ����Ϊ��Щ�ʽ������� ʡ��
    stop_words_={
        "a","the","is","as","at","an","this","what","that","will","are","were","was"
    };
}


std::vector<std::string> Tokenizer::tokenize(const std::string& text) const
{
    //����ͨ�ı����зִ�
    //1. ���ո�ָ�
    //2. ת��ΪСд
    //3. ȥ��������
    
    std::vector<std::string> result;
    std::string current_token;

    for (char ch : text)
    {
        if (std::isalpha(ch) || std::isdigit(ch))
        {
            //�ռ���ĸ������
            current_token += std::tolower(ch);
        }
        else if (!current_token.empty())
        {
            //���������ַ�ʱ�����浱ǰtoken
            result.push_back(current_token);
            current_token.clear();
        }
    }

    //�������һ��token
    if (!current_token.empty())
    {
        result.push_back(current_token);
    }

    return result;

    //return perform_tokenization(text,true);
}



std::vector<std::string> Tokenizer::tokenize_raw_query(const std::string& text) const
{
    //�Բ�ѯ�ַ������зִ�
    //���⴦��
    //1. ����AND��OR��NOT�ȹؼ���
    //2. ֧�������ڵĶ���
    //3. ����ԭʼ��Сд������ʶ��ؼ��ʣ�
    
    std::vector<std::string> tokens;
    std::istringstream iss(text);
    std::string token;

    //�򵥰��ո�ִʣ�����ԭʼ��ʽ
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
