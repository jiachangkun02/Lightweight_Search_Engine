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
        //ͣ�ô��б� ˵�˻� ���ǽ�һЩ��Ƶ���ֵ�����Ҫ�Ĵ���ӽ�ȥ ����Ϊ��Щ�ʽ������� ʡ��
        Tokenizer();

        //���ı��и�
        std::vector<std::string> tokenize(const std::string& text) const;

        std::vector<std::string> tokenize_raw_query(const std::string& text) const;

        private
        :
        std::unordered_set<std::string> stop_words_;


        std::vector<std::string> perform_tokenization(const std::string& text, bool remove_stopwords)const;
    };
}

