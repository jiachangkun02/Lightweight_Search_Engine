#include "InvertedIndex.h"
#include <fstream>
#include <sstream>
#include <iostream>


namespace jia
{
    // 构造函数：初始化分词器
    // 使用智能指针管理分词器实例，避免内存泄漏
    InvertedIndex::InvertedIndex()
        :tokenizer_(std::make_unique<Tokenizer>())
    {

    }

    
    // 添加文档到索引中
    // 1. 对文档内容进行分词
    // 2. 记录文档信息（路径和长度）
    // 3. 统计词频
    // 4. 更新倒排索引表
    // 返回新文档的ID
    DocId InvertedIndex::add_doc(const std::string& path, const std::string& content)
    {
        DocId current_doc_id = get_doc_count();

        // 对文档内容进行分词处理
        std::vector<std::string> tokens=tokenizer_->tokenize(content);

        // 保存文档的元信息（路径和总词数）
        DocsInFcontainer_.push_back(DocInF{ path,static_cast<int>(tokens.size()) });

        // 统计每个词在文档中出现的次数
        std::unordered_map<std::string, int> term_frequencies;
        for (const auto& token : tokens)
        {
            term_frequencies[token]++; // 遇到重复的词就增加计数
        }

        // 将统计结果更新到倒排索引中
        for (const auto& pair : term_frequencies)
        {
            const std::string& term = pair.first;
            int tf = pair.second;
            
            // 添加新的倒排索引项
            index_[term].push_back(PostingEntry{ current_doc_id,tf });
        }

        return current_doc_id;
    }

    
    // 查询某个词的倒排索引表项
    // 返回包含该词的所有文档及其出现次数信息
    // 如果词不存在，返回nullptr
    const std::vector<PostingEntry>* InvertedIndex::get_posting_entries(const std::string& term) const
    {
        auto it = index_.find(term);

        if (it==index_.end())
        {
            return nullptr; // 如果没找到，返回空指针
        }

        return &(it->second); // 返回PostingEntry列表的引用
    }


    // 获取索引中的总文档数
    size_t InvertedIndex::get_doc_count() const
    {
        return DocsInFcontainer_.size();
    }

    
    // 根据文档ID获取文档的元信息（路径和长度）
    const DocInF& InvertedIndex::get_docINF(DocId doc_id) const
    {
        return DocsInFcontainer_[doc_id];
    }

    
    // 获取包含指定词的文档数量
    // 用于计算逆文档频率(IDF)
    size_t InvertedIndex::get_term_frequency_inDoc(const std::string& term) const
    {
         auto it=index_.find(term);

         if (it==index_.end())
         {
             return 0;
         }
         return it->second.size();
    }
    
    
    // 使用boost序列化库保存索引到文件
    // 将整个索引结构（包括文档信息和倒排表）序列化到二进制文件
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
            // boost会自动调用类中定义的serialize方法序列化所有成员
            oa << (*this);
        }
        catch (const std::exception& e) {
            std::cerr << "Boost serialization save error: " << e.what() << std::endl;
            return false;
        }
        return true;
    }

    
    // 从文件加载序列化的索引数据
    // 1. 清空当前索引
    // 2. 从文件读取并反序列化
    // 3. 使用boost的反序列化机制还原索引结构
    bool InvertedIndex::load_boost(const std::string& filepath)
    {
        try {
            std::ifstream ifs(filepath, std::ios::binary);
            if (!ifs) {
                std::cerr << "Error: Cannot open file for reading: " << filepath << std::endl;
                return false;
            }
            boost::archive::binary_iarchive ia(ifs);
            
            // 清空现有数据，准备加载
            DocsInFcontainer_.clear();
            index_.clear();
            
            // 从文件加载并反序列化
            ia >> (*this);
        }
        catch (const std::exception& e) {
            std::cerr << "Boost serialization load error: " << e.what() << std::endl;
            return false;
        }
        return true;
    }
}


