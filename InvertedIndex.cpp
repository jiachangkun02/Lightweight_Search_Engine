#include "InvertedIndex.h"
#include <fstream>
#include <sstream>
#include <iostream>


namespace jia
{
    // ���캯������ʼ���ִ���
    // ʹ������ָ�����ִ���ʵ���������ڴ�й©
    InvertedIndex::InvertedIndex()
        :tokenizer_(std::make_unique<Tokenizer>())
    {

    }

    
    // ����ĵ���������
    // 1. ���ĵ����ݽ��зִ�
    // 2. ��¼�ĵ���Ϣ��·���ͳ��ȣ�
    // 3. ͳ�ƴ�Ƶ
    // 4. ���µ���������
    // �������ĵ���ID
    DocId InvertedIndex::add_doc(const std::string& path, const std::string& content)
    {
        DocId current_doc_id = get_doc_count();

        // ���ĵ����ݽ��зִʴ���
        std::vector<std::string> tokens=tokenizer_->tokenize(content);

        // �����ĵ���Ԫ��Ϣ��·�����ܴ�����
        DocsInFcontainer_.push_back(DocInF{ path,static_cast<int>(tokens.size()) });

        // ͳ��ÿ�������ĵ��г��ֵĴ���
        std::unordered_map<std::string, int> term_frequencies;
        for (const auto& token : tokens)
        {
            term_frequencies[token]++; // �����ظ��Ĵʾ����Ӽ���
        }

        // ��ͳ�ƽ�����µ�����������
        for (const auto& pair : term_frequencies)
        {
            const std::string& term = pair.first;
            int tf = pair.second;
            
            // ����µĵ���������
            index_[term].push_back(PostingEntry{ current_doc_id,tf });
        }

        return current_doc_id;
    }

    
    // ��ѯĳ���ʵĵ�����������
    // ���ذ����ôʵ������ĵ�������ִ�����Ϣ
    // ����ʲ����ڣ�����nullptr
    const std::vector<PostingEntry>* InvertedIndex::get_posting_entries(const std::string& term) const
    {
        auto it = index_.find(term);

        if (it==index_.end())
        {
            return nullptr; // ���û�ҵ������ؿ�ָ��
        }

        return &(it->second); // ����PostingEntry�б������
    }


    // ��ȡ�����е����ĵ���
    size_t InvertedIndex::get_doc_count() const
    {
        return DocsInFcontainer_.size();
    }

    
    // �����ĵ�ID��ȡ�ĵ���Ԫ��Ϣ��·���ͳ��ȣ�
    const DocInF& InvertedIndex::get_docINF(DocId doc_id) const
    {
        return DocsInFcontainer_[doc_id];
    }

    
    // ��ȡ����ָ���ʵ��ĵ�����
    // ���ڼ������ĵ�Ƶ��(IDF)
    size_t InvertedIndex::get_term_frequency_inDoc(const std::string& term) const
    {
         auto it=index_.find(term);

         if (it==index_.end())
         {
             return 0;
         }
         return it->second.size();
    }
    
    
    // ʹ��boost���л��Ᵽ���������ļ�
    // �����������ṹ�������ĵ���Ϣ�͵��ű����л����������ļ�
    bool InvertedIndex::save_boost(const std::string& filepath)
    {
        /*std::ofstream ofs(filepath, std::ios::binary);

        if (!ofs)
        {
            return false;
        }


        size_t doc_count = DocsInFcontainer_.size();

        ofs.write(reinterpret_cast<const char*>(&doc_count), sizeof(doc_count));

        for (const auto& doc : DocsInFcontainer_)
        {
            size_t path_length = doc.doc_path.size();

            
        }*/


        try {
            std::ofstream ofs(filepath, std::ios::binary);
            if (!ofs) {
                std::cerr << "Error: Cannot open file for writing: " << filepath << std::endl;
                return false;
            }
            boost::archive::binary_oarchive oa(ofs);
            // boost���Զ��������ж����serialize�������л����г�Ա
            oa << (*this);
        }
        catch (const std::exception& e) {
            std::cerr << "Boost serialization save error: " << e.what() << std::endl;
            return false;
        }
        return true;
    }

    
    // ���ļ��������л�����������
    // 1. ��յ�ǰ����
    // 2. ���ļ���ȡ�������л�
    // 3. ʹ��boost�ķ����л����ƻ�ԭ�����ṹ
    bool InvertedIndex::load_boost(const std::string& filepath)
    {
        try {
            std::ifstream ifs(filepath, std::ios::binary);
            if (!ifs) {
                std::cerr << "Error: Cannot open file for reading: " << filepath << std::endl;
                return false;
            }
            boost::archive::binary_iarchive ia(ifs);
            
            // ����������ݣ�׼������
            DocsInFcontainer_.clear();
            index_.clear();
            
            // ���ļ����ز������л�
            ia >> (*this);
        }
        catch (const std::exception& e) {
            std::cerr << "Boost serialization load error: " << e.what() << std::endl;
            return false;
        }
        return true;
    }
}


