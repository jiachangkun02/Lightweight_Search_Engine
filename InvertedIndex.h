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
        // 详细说明：初始化倒排索引对象，创建分词器实例，设置初始状态
        // 参数：无
        // 返回值：无

        //将一篇文档添加到索引中去
        DocId add_doc(const std::string& path,const std::string& content);
        // 详细说明：将文档内容添加到倒排索引中，包括分词、统计词频、更新索引表
        // 参数：
        //   path - 文档的完整路径，用于标识文档和防止重复索引
        //   content - 文档的文本内容，将被分词处理
        // 返回值：新文档的ID，如果文档已存在则返回现有ID

        //获取一个词的发布列表
        const std::vector<PostingEntry>* get_posting_entries(const std::string& term) const;
        // 详细说明：查询指定词汇的倒排索引项，返回包含该词的所有文档及其位置信息
        // 参数：
        //   term - 要查询的词汇，将转换为小写进行匹配
        // 返回值：指向PostingEntry列表的指针，如果词不存在则返回nullptr

        //获取总的文档数
        size_t get_doc_count() const;
        // 详细说明：返回当前索引中包含的文档总数，用于统计和IDF计算
        // 参数：无
        // 返回值：文档数量，类型为size_t


        //获取包含某个特定词的文档数量
        size_t get_term_frequency_inDoc(const std::string& term) const;
        // 详细说明：计算包含指定词汇的文档数量，用于逆文档频率(IDF)计算
        // 参数：
        //   term - 要查询的词汇
        // 返回值：包含该词的文档数量，如果词不存在则返回0

        //获取某个指定文档的信息
        const DocInF& get_docINF(DocId doc_id) const;
        // 详细说明：根据文档ID获取文档的元信息，包括路径和长度
        // 参数：
        //   doc_id - 文档的唯一标识符
        // 返回值：DocInF结构体的常量引用，包含文档路径和长度信息
		
        //  获取平均文档长度
        double get_avg_doc_length() const; //
        // 详细说明：计算所有文档的平均长度，用于BM25排序算法的长度归一化
        // 参数：无
        // 返回值：平均文档长度，如果没有文档则返回0.0
		
        //  获取唯一词汇数量
        size_t get_unique_term_count() const; //
        // 详细说明：返回索引中不同词汇的总数，用于统计展示和调试
        // 参数：无
        // 返回值：唯一词汇的数量
		
        //  获取总词数
        size_t get_total_terms() const; //
        // 详细说明：返回所有文档中词汇出现的总次数，用于统计和长度归一化
        // 参数：无
        // 返回值：所有文档中词汇的总出现次数
		
        //  获取全部 doc id 列表
        std::vector<DocId> get_all_doc_ids() const; //
        // 详细说明：生成包含所有文档ID的列表，为NOT运算提供全集
        // 参数：无
        // 返回值：包含所有文档ID的向量
		
        //  判断文档是否已被索引
        bool has_doc(const std::string& path) const; //
        // 详细说明：检查指定路径的文档是否已经被索引，防止重复索引
        // 参数：
        //   path - 文档的完整路径
        // 返回值：如果文档已存在返回true，否则返回false

        //将编制好的索引保存到文件之中
        bool save_boost(const std::string& filepath);
        // 详细说明：使用boost序列化库将完整索引结构保存到二进制文件
        // 参数：
        //   filepath - 保存文件的路径
        // 返回值：保存成功返回true，失败返回false
		
        //  紧凑版保存/加载
        bool save_compact(const std::string& filepath) const; //
        // 详细说明：使用紧凑格式保存索引，包括delta编码doc_id和位置信息
        // 参数：
        //   filepath - 保存文件的路径
        // 返回值：保存成功返回true，失败返回false
		
        bool load_compact(const std::string& filepath); //
        // 详细说明：从紧凑格式文件加载索引，重建文档列表和倒排表
        // 参数：
        //   filepath - 加载文件的路径
        // 返回值：加载成功返回true，失败返回false
		
        //  清空索引
        void clear(); //
        // 详细说明：清空内存中的所有索引数据，释放倒排表、文档信息和路径映射
        // 参数：无
        // 返回值：无

        //从文件中加载以保存进去的索引
        bool load_boost(const std::string& filepath);
        // 详细说明：使用boost反序列化从文件加载索引，加载后重建统计信息和路径映射
        // 参数：
        //   filepath - 加载文件的路径
        // 返回值：加载成功返回true，失败返回false
        
    private:

        //建立单个词与发布列表的映射关系
        std::unordered_map<std::string, std::vector<PostingEntry>> index_;
        // 详细说明：核心倒排索引数据结构，存储词汇到发布列表的映射
        // 键：词汇字符串（小写形式）
        // 值：PostingEntry向量，包含文档ID、词频和位置信息

        //用来存储文档信息的数组
        std::vector<DocInF> DocsInFcontainer_;
        // 详细说明：存储所有文档的元信息，按文档ID顺序排列
        // 每个元素包含文档路径和文档长度（词汇数量）
        //  跟踪总词数
        size_t total_terms_ = 0; //
        // 详细说明：记录所有文档中词汇出现的总次数，用于计算平均文档长度
        // 在add_doc时累加，在clear时重置为0
        //  路径到 doc_id 的映射，便于去重
        std::unordered_map<std::string, DocId> path_to_id_; //
        // 详细说明：维护文档路径到文档ID的映射，用于快速检查文档是否已索引
        // 键：文档完整路径
        // 值：对应的文档ID

        //分词器实例
        std::unique_ptr<Tokenizer> tokenizer_;
        // 详细说明：智能指针管理的分词器对象，负责将文本内容分割为词汇
        // 使用unique_ptr确保资源自动释放，避免内存泄漏


        friend class boost::serialization::access;
        // 详细说明：声明boost序列化库为友元类，允许访问私有成员进行序列化

        template<class archive>
        void serialize(archive & ar, const unsigned int version)
        {
            // 详细说明：boost序列化模板函数，用于对象的序列化和反序列化
            // 参数：
            //   ar - 序列化存档对象，可以是输入存档或输出存档
            //   version - 序列化版本号，用于版本兼容性处理
            // 功能：序列化所有关键数据成员，确保索引状态完整保存和恢复
			
            ar & index_;			// 序列化核心倒排索引表
            
            ar & DocsInFcontainer_;	// 序列化文档信息容器
            
            ar & total_terms_;     	// 序列化总词数统计
			
            ar & path_to_id_; 		// 序列化路径到ID的映射表
            
        }

        
    };
}

