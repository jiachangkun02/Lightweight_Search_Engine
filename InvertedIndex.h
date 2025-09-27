#pragma once
#include "Tokenizer.h"
#include "IndexData.h"
#include <memory>

namespace jia
{
    class InvertedIndex
    {
    public:
        //构造函数
        InvertedIndex();

        //将一篇文档添加到索引中去
        DocId add_doc(const std::string& path,const std::string& content);

        //获取一个词的发布列表
        const std::vector<PostingEntry>* get_posting_entries(const std::string& term) const;

        //获取总的文档数
        size_t get_doc_count() const;


        //获取包含某个特定词的文档数量
        size_t get_term_frequency_inDoc(const std::string& term) const;

        //获取某个指定文档的信息
        const DocInF& get_docINF(DocId doc_id) const;

        //将编制好的索引保存到文件之中
        bool save_boost(const std::string& filepath);

        //从文件中加载以保存进去的索引
        bool load_boost(const std::string& filepath);
        
    private:

        //建立单个词与发布列表的映射关系
        std::unordered_map<std::string, std::vector<PostingEntry>> index_;

        //用来存储文档信息的数组
        std::vector<DocInF> DocsInFcontainer_;

        //分词器实例
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

