#pragma once
#include "Tokenizer.h"
#include "IndexData.h"
#include <memory>

namespace jia
{
    class InvertedIndex
    {
    public:
        //���캯��
        InvertedIndex();

        //��һƪ�ĵ���ӵ�������ȥ
        DocId add_doc(const std::string& path,const std::string& content);

        //��ȡһ���ʵķ����б�
        const std::vector<PostingEntry>* get_posting_entries(const std::string& term) const;

        //��ȡ�ܵ��ĵ���
        size_t get_doc_count() const;


        //��ȡ����ĳ���ض��ʵ��ĵ�����
        size_t get_term_frequency_inDoc(const std::string& term) const;

        //��ȡĳ��ָ���ĵ�����Ϣ
        const DocInF& get_docINF(DocId doc_id) const;

        //�����ƺõ��������浽�ļ�֮��
        bool save_boost(const std::string& filepath);

        //���ļ��м����Ա����ȥ������
        bool load_boost(const std::string& filepath);
        
    private:

        //�����������뷢���б��ӳ���ϵ
        std::unordered_map<std::string, std::vector<PostingEntry>> index_;

        //�����洢�ĵ���Ϣ������
        std::vector<DocInF> DocsInFcontainer_;

        //�ִ���ʵ��
        std::unique_ptr<Tokenizer> tokenizer_;


        friend class boost::serialization::access;

        template<class archive>
        void serialize(archive & ar, const unsigned int version)
        {
            ar & index_;
            ar & DocsInFcontainer_;
        }

        
    };
}

