#pragma once
#include <string>
#include <unordered_map>
#include <vector>
// boost序列化相关的头文件，用于持久化索引数据
#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>


// 搜索引擎核心数据结构命名空间
namespace jia
{
    // 用整型表示文档ID，方便索引和查找
    using DocId = int;

    // 倒排索引项结构体 - 记录单个term在某个文档中的出现情况
    struct PostingEntry
    {
        DocId doc_id_;              // 文档ID
        int frequency;              // 统计词在文件中出现的次数
        std::vector<int> position;  // 词在文档中的下标位置，保留位置信息便于后续支持短语查询


    private:
        // boost序列化支持 - 用于保存和加载索引
        friend class boost::serialization::access;

        template<class archive>
        void serialize(archive & ar, const unsigned int version)
        {
            ar & doc_id_;
            ar & frequency;
            ar & position;
        }
        
        
    };

    // 文档元信息结构体 - 保存文档的基本信息
    struct DocInF
    {
        std::string doc_path;    // 文档路径
        int length;              // 整个文档的总词数，为什么要有这个呢？背后的背景知识我待会详细的介绍的

    private:
        // boost序列化支持 - 用于保存和加载文档信息
        friend class boost::serialization::access;

        template<class archive>
        void serialize(archive & ar, const unsigned int version)
        {
            ar& length;
            ar& doc_path;
        }
        
    };

    // 带分数的文档结构体 - 用于返回排序后的搜索结果
    struct ScoredDoc
    {
        DocId doc_id;       // 文档ID
        double score;       // 文档的相关性得分

        // 重载>运算符实现基于分数的排序（降序）
        bool operator>(const ScoredDoc& other) const
        {
            return score > other.score;
        }
        
    };
    
}