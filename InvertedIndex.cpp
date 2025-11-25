#include "InvertedIndex.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace jia
{
    // 构造函数：初始化分词器
    // 使用智能指针管理分词器实例，避免内存泄漏
    InvertedIndex::InvertedIndex()
        :tokenizer_(std::make_unique<Tokenizer>())
    {
        // 详细说明：构造函数初始化所有成员变量
        // 使用std::make_unique创建分词器实例，确保资源自动管理
        // 其他成员变量使用默认初始化：
        //   index_ - 空的unordered_map
        //   DocsInFcontainer_ - 空的vector
        //   total_terms_ - 初始化为0
        //   path_to_id_ - 空的unordered_map
    }


    // 添加文档到索引中
    // 1. 对文档内容进行分词
    // 2. 记录文档信息（路径和长度）
    // 3. 统计词频
    // 4. 更新倒排索引表
    // 返回新文档的ID
    DocId InvertedIndex::add_doc(const std::string& path, const std::string& content)
    {
        // 详细说明：核心索引构建函数，将文档内容处理并添加到倒排索引中
        // 执行流程：
        //   1. 检查文档是否已存在，避免重复索引
        //   2. 分配新的文档ID
        //   3. 对文档内容进行分词
        //   4. 保存文档元信息
        //   5. 统计词频和位置信息
        //   6. 更新倒排索引表

        //  避免重复索引同一路径
        if (has_doc(path))
        {
            // 详细说明：如果文档路径已存在，直接返回现有文档ID
            // 这确保了同一文档不会被重复索引，提高索引效率
            return path_to_id_[path];
        }
        DocId current_doc_id = get_doc_count();
        // 详细说明：使用当前文档数量作为新文档的ID
        // 文档ID从0开始连续分配，便于后续的数组索引

        // 对文档内容进行分词处理
        std::vector<std::string> tokens=tokenizer_->tokenize(content);
        // 详细说明：调用分词器将文本内容分割为词汇列表
        // 分词过程包括：文本清理、词汇分割、停用词过滤等

        // 保存文档的元信息（路径和总词数）
        DocsInFcontainer_.push_back(DocInF{ path,static_cast<int>(tokens.size()) });
        // 详细说明：将文档路径和词汇数量保存到文档信息容器中
        total_terms_ += tokens.size();
        // 详细说明：累加总词数，用于后续计算平均文档长度
        path_to_id_[path] = current_doc_id;
        // 详细说明：建立路径到文档ID的映射，便于快速查找

        // 统计每个词在文档中出现的次数
        // std::unordered_map<std::string, int> term_frequencies;
        // for (const auto& token : tokens)
        // {
        //     term_frequencies[token]++; // 遇到重复的词就增加计数
        // }
        //
        // // 将统计结果更新到倒排索引中
        // for (const auto& pair : term_frequencies)
        // {
        //     const std::string& term = pair.first;
        //     int tf = pair.second;
        //
        //     // 添加新的倒排索引项
        //     index_[term].push_back(PostingEntry{ current_doc_id,tf });
        // }
        //  记录词频同时保存位置，方便后续支持短语/邻近查询
        std::unordered_map<std::string, PostingEntry> term_postings;
        // 详细说明：临时映射表，记录每个词汇在文档中的统计信息
        // 键：词汇字符串
        // 值：PostingEntry对象，包含文档ID、词频和位置列表
        for (size_t pos = 0; pos < tokens.size(); ++pos)
        {
            const std::string& token = tokens[pos];
            auto& entry = term_postings[token];
            entry.doc_id_ = current_doc_id;
            entry.frequency++;
            entry.position.push_back(static_cast<int>(pos));
            // 详细说明：为每个词汇记录：
            //   - 文档ID
            //   - 词频（出现次数）
            //   - 在文档中的位置列表
        }
        for (auto& pair : term_postings)
        {
            index_[pair.first].push_back(pair.second);
            // 详细说明：将临时统计结果合并到主倒排索引中
            // 每个词汇的PostingEntry被添加到对应的倒排列表
        }

        return current_doc_id;
        // 详细说明：返回新分配的文档ID，调用者可以用此ID引用该文档
    }


    // 查询某个词的倒排索引表项
    // 返回包含该词的所有文档及其出现次数信息
    // 如果词不存在，返回nullptr
    const std::vector<PostingEntry>* InvertedIndex::get_posting_entries(const std::string& term) const
    {
        // 详细说明：查询指定词汇的倒排索引项，这是搜索操作的核心函数
        // 参数term应该已经是小写形式，以确保与索引中的词汇匹配

        auto it = index_.find(term);
        // 详细说明：在倒排索引表中查找指定词汇
        // 使用unordered_map的find操作，平均时间复杂度为O(1)

        if (it==index_.end())
        {
            return nullptr; // 如果没找到，返回空指针
            // 详细说明：词汇不存在于索引中，返回nullptr表示查询失败
        }

        return &(it->second); // 返回PostingEntry列表的引用
        // 详细说明：返回指向PostingEntry向量的指针
        // 调用者可以访问包含该词的所有文档及其位置信息
    }


    // 获取索引中的总文档数：用于统计和 IDF 计算。
    size_t InvertedIndex::get_doc_count() const
    {
        return DocsInFcontainer_.size();
    }

    
    // 根据文档ID获取文档元信息（路径、长度）：用于展示结果与长度归一化。
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

    //  获取平均文档长度
    
	// 获取平均文档长度：用于 BM25 长度归一化。    
	double InvertedIndex::get_avg_doc_length() const
    {
        if (DocsInFcontainer_.empty())
        {
            return 0.0;
        }
        return static_cast<double>(total_terms_) / static_cast<double>(DocsInFcontainer_.size());
    }

    //  获取唯一词汇数量
    
	// 获取唯一词汇数：用于统计展示与调试。    
	size_t InvertedIndex::get_unique_term_count() const
    {
        return index_.size();
    }

    //  获取总词数
    
	// 获取总词数：用于统计与长度归一化。    
	size_t InvertedIndex::get_total_terms() const
    {
        return total_terms_;
    }

    //  获取全部 doc id 列表
    // 获取所有文档 ID：为 NOT 运算提供全集。    
	std::vector<DocId> InvertedIndex::get_all_doc_ids() const
    {
        std::vector<DocId> ids;
        ids.reserve(DocsInFcontainer_.size());
        for (DocId i = 0; i < static_cast<DocId>(DocsInFcontainer_.size()); ++i)
        {
            ids.push_back(i);
        }
        return ids;
    }

    //  判断文档是否已经存在

	// 判断文档是否已存在：用于防止重复索引同一路径。    
	bool InvertedIndex::has_doc(const std::string& path) const
    {
        return path_to_id_.find(path) != path_to_id_.end();
    }
    
    
    // 使用boost序列化库保存索引到文件
    // 将整个索引结构（包括文档信息和倒排表）序列化到二进制文件
    // 使用 boost 序列化保存完整索引，包含文档元信息与倒排表。
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

    //  清空索引
    // 清空内存索引：释放倒排表、文档信息、路径映射和总词数。    
	void InvertedIndex::clear()
    {
        index_.clear();
        DocsInFcontainer_.clear();
        path_to_id_.clear();
        total_terms_ = 0;
    }

    //  简单紧凑格式保存（delta 编码 doc_id）
    // 保存紧凑索引：写出 term 数、doc 数、总词数，文档元信息，term 的 delta 编码 doc_id 与位置信息。
	bool InvertedIndex::save_compact(const std::string& filepath) const
    {
        std::ofstream ofs(filepath, std::ios::binary);
        if (!ofs)
        {
            std::cerr << "Error: Cannot open file for writing: " << filepath << std::endl;
            return false;
        }
        size_t term_count = index_.size();
        size_t doc_count = DocsInFcontainer_.size();
        ofs.write(reinterpret_cast<const char*>(&term_count), sizeof(term_count));
        ofs.write(reinterpret_cast<const char*>(&doc_count), sizeof(doc_count));
        ofs.write(reinterpret_cast<const char*>(&total_terms_), sizeof(total_terms_));
        // 写文档信息
        for (const auto& doc : DocsInFcontainer_)
        {
            size_t path_len = doc.doc_path.size();
            ofs.write(reinterpret_cast<const char*>(&path_len), sizeof(path_len));
            ofs.write(doc.doc_path.data(), static_cast<std::streamsize>(path_len));
            ofs.write(reinterpret_cast<const char*>(&doc.length), sizeof(doc.length));
        }
        // 写倒排索引
        for (const auto& kv : index_)
        {
            const std::string& term = kv.first;
            size_t term_len = term.size();
            ofs.write(reinterpret_cast<const char*>(&term_len), sizeof(term_len));
            ofs.write(term.data(), static_cast<std::streamsize>(term_len));
            // delta 编码 doc_id
            std::vector<PostingEntry> postings = kv.second;
            std::sort(postings.begin(), postings.end(), [](const PostingEntry& a, const PostingEntry& b)
            {
                return a.doc_id_ < b.doc_id_;
            });
            size_t postings_size = postings.size();
            ofs.write(reinterpret_cast<const char*>(&postings_size), sizeof(postings_size));
            int last_id = 0;
            for (const auto& entry : postings)
            {
                int delta = entry.doc_id_ - last_id;
                last_id = entry.doc_id_;
                ofs.write(reinterpret_cast<const char*>(&delta), sizeof(delta));
                ofs.write(reinterpret_cast<const char*>(&entry.frequency), sizeof(entry.frequency));
                size_t pos_count = entry.position.size();
                ofs.write(reinterpret_cast<const char*>(&pos_count), sizeof(pos_count));
                for (int pos : entry.position)
                {
                    ofs.write(reinterpret_cast<const char*>(&pos), sizeof(pos));
                }
            }
        }
        return true;
    }

    //  紧凑格式加载
    // 加载紧凑索引：按保存顺序重建文档列表、路径映射与倒排表，恢复总词数。
	bool InvertedIndex::load_compact(const std::string& filepath)
    {
        std::ifstream ifs(filepath, std::ios::binary);
        if (!ifs)
        {
            std::cerr << "Error: Cannot open file for reading: " << filepath << std::endl;
            return false;
        }
        clear();
        size_t term_count = 0;
        size_t doc_count = 0;
        ifs.read(reinterpret_cast<char*>(&term_count), sizeof(term_count));
        ifs.read(reinterpret_cast<char*>(&doc_count), sizeof(doc_count));
        ifs.read(reinterpret_cast<char*>(&total_terms_), sizeof(total_terms_));
        for (size_t i = 0; i < doc_count; ++i)
        {
            size_t path_len = 0;
            ifs.read(reinterpret_cast<char*>(&path_len), sizeof(path_len));
            std::string path(path_len, '\0');
            ifs.read(&path[0], static_cast<std::streamsize>(path_len));
            int length = 0;
            ifs.read(reinterpret_cast<char*>(&length), sizeof(length));
            DocsInFcontainer_.push_back(DocInF{ path, length });
            path_to_id_[path] = static_cast<DocId>(i);
        }
        for (size_t t = 0; t < term_count; ++t)
        {
            size_t term_len = 0;
            ifs.read(reinterpret_cast<char*>(&term_len), sizeof(term_len));
            std::string term(term_len, '\0');
            ifs.read(&term[0], static_cast<std::streamsize>(term_len));
            size_t postings_size = 0;
            ifs.read(reinterpret_cast<char*>(&postings_size), sizeof(postings_size));
            std::vector<PostingEntry> postings;
            postings.reserve(postings_size);
            int last_id = 0;
            for (size_t p = 0; p < postings_size; ++p)
            {
                int delta = 0;
                ifs.read(reinterpret_cast<char*>(&delta), sizeof(delta));
                int doc_id = last_id + delta;
                last_id = doc_id;
                int freq = 0;
                ifs.read(reinterpret_cast<char*>(&freq), sizeof(freq));
                size_t pos_count = 0;
                ifs.read(reinterpret_cast<char*>(&pos_count), sizeof(pos_count));
                std::vector<int> positions;
                positions.reserve(pos_count);
                for (size_t k = 0; k < pos_count; ++k)
                {
                    int pos = 0;
                    ifs.read(reinterpret_cast<char*>(&pos), sizeof(pos));
                    positions.push_back(pos);
                }
                postings.push_back(PostingEntry{ static_cast<DocId>(doc_id), freq, positions });
            }
            index_[term] = std::move(postings);
        }
        return true;
    }

    
    // 从文件加载序列化的索引数据
    // 1. 清空当前索引
    // 2. 从文件读取并反序列化
    // 3. 使用boost的反序列化机制还原索引结构
    // 使用 boost 反序列化加载索引，加载后重建缺失的统计与路径映射。
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
            total_terms_ = 0;
            path_to_id_.clear();
            
            // 从文件加载并反序列化
            ia >> (*this);
            //  兼容旧数据，如果total_terms_为0则重算
            if (total_terms_ == 0)
            {
                for (const auto& doc : DocsInFcontainer_)
                {
                    total_terms_ += doc.length;
                }
            }
            //  重新生成路径映射
            if (path_to_id_.empty())
            {
                for (DocId i = 0; i < static_cast<DocId>(DocsInFcontainer_.size()); ++i)
                {
                    path_to_id_[DocsInFcontainer_[i].doc_path] = i;
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Boost serialization load error: " << e.what() << std::endl;
            return false;
        }
        return true;
    }
}


