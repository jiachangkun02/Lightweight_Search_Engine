#pragma once
#include "IndexData.h"
#include "InvertedIndex.h"
using namespace std;

namespace jia
{
    class Tokenizer;
    
    
    class QueryProcessor
    {
    public:
        explicit QueryProcessor(InvertedIndex& index);
        // 详细说明：构造函数，初始化查询处理器并关联到倒排索引
        // 参数：
        //   index - 倒排索引的引用，查询处理器将使用此索引进行搜索
        // 返回值：无

        // std::vector<ScoredDoc> process(const std::string& terms); //
        //  支持带选项的查询入口
        struct QueryOptions 
        { 
            bool use_bm25 = false; 
            // 详细说明：是否使用BM25排序算法，false表示使用TF-IDF
			
            size_t top_k = 50; 
            // 详细说明：返回结果的最大数量，用于分页和性能优化
			
            size_t offset = 0; 
            // 详细说明：结果偏移量，用于分页显示
        }; 
        // 详细说明：查询选项结构体，封装了查询的各种配置参数
		
        std::vector<ScoredDoc> process(const std::string& terms, const QueryOptions& options = QueryOptions()); 
        // 详细说明：对外查询入口函数，接收查询字符串和选项参数
        // 参数：
        //   terms - 查询字符串，支持布尔操作符和短语查询
        //   options - 查询选项，包含排序算法、结果数量等配置
        // 返回值：排序后的文档列表，按相关性得分降序排列
        
    private:

        //计算某个词的排名结果依据TF—IDF算法
        std::vector<ScoredDoc> rank_TF_IDF(const std::vector<std::string>& terms) const;
        // 详细说明：使用TF-IDF算法对查询词汇集合进行排序
        // 参数：
        //   terms - 查询词汇列表
        // 返回值：按TF-IDF得分排序的文档列表
		
        //  依据 BM25 的排名函数
        std::vector<ScoredDoc> rank_BM25(const std::vector<std::string>& terms) const; 
        // 详细说明：使用BM25算法对查询词汇集合进行排序
        // 参数：
        //   terms - 查询词汇列表
        // 返回值：按BM25得分排序的文档列表
		
        //  自动选择 TF-IDF 或 BM25
        std::vector<ScoredDoc> rank_terms(const std::vector<std::string>& terms, bool use_bm25) const; 
        // 详细说明：根据use_bm25标志选择TF-IDF或BM25排序算法
        // 参数：
        //   terms - 查询词汇列表
        //   use_bm25 - 是否使用BM25算法
        // 返回值：按选定算法排序的文档列表
		
        //  短语查询（连续匹配）
        std::vector<ScoredDoc> rank_phrase(const std::vector<std::string>& phrase_terms) const; 
        // 详细说明：处理短语查询，要求词汇在文档中连续出现
        // 参数：
        //   phrase_terms - 短语中的词汇列表
        // 返回值：包含完整短语的文档及其匹配次数

        std::vector<ScoredDoc> evaluate_AND_NOT_query(const std::vector<std::string>& tokens);
        // 详细说明：评估AND和NOT组合的简单布尔查询
        // 参数：
        //   tokens - 分词后的查询令牌
        // 返回值：满足AND和NOT条件的文档列表


        // std::vector<ScoredDoc> evaluate_boolean_query(const std::string& query_string); 
        std::vector<ScoredDoc> evaluate_boolean_query(const std::string& query_string, const QueryOptions& options); 
        // 详细说明：完整的布尔查询评估器，支持AND/OR/NOT和括号
        // 参数：
        //   query_string - 原始查询字符串
        //   options - 查询选项
        // 返回值：布尔查询的结果文档列表
		
        std::vector<ScoredDoc> evaluate_expression(const std::vector<std::string>& tokens, size_t& pos, const QueryOptions& options); 
        // 详细说明：递归评估布尔表达式（处理OR操作）
        // 参数：
        //   tokens - 分词后的查询令牌
        //   pos - 当前处理位置的引用
        //   options - 查询选项
        // 返回值：表达式评估结果
		
        std::vector<ScoredDoc> evaluate_term(const std::vector<std::string>& tokens, size_t& pos, const QueryOptions& options); 
        // 详细说明：评估布尔项（处理AND操作）
        // 参数：
        //   tokens - 分词后的查询令牌
        //   pos - 当前处理位置的引用
        //   options - 查询选项
        // 返回值：项评估结果
		
        std::vector<ScoredDoc> evaluate_factor(const std::vector<std::string>& tokens, size_t& pos, const QueryOptions& options); 
        // 详细说明：评估布尔因子（处理NOT操作和基本查询单元）
        // 参数：
        //   tokens - 分词后的查询令牌
        //   pos - 当前处理位置的引用
        //   options - 查询选项
        // 返回值：因子评估结果
		
        std::vector<ScoredDoc> vector_to_scored(const std::unordered_map<DocId, double>& m) const; 
        // 详细说明：将文档得分映射转换为ScoredDoc向量
        // 参数：
        //   m - 文档ID到得分的映射
        // 返回值：排序后的ScoredDoc向量
		
        std::unordered_map<DocId, double> scored_to_map(const std::vector<ScoredDoc>& v) const; 
        // 详细说明：将ScoredDoc向量转换为文档得分映射
        // 参数：
        //   v - ScoredDoc向量
        // 返回值：文档ID到得分的映射
        std::unordered_map<DocId, double> merge_or(const std::unordered_map<DocId, double>& a, const std::unordered_map<DocId, double>& b) const; 
        // 详细说明：合并两个文档集合（OR操作）
        // 参数：
        //   a, b - 要合并的文档得分映射
        // 返回值：合并后的文档得分映射
		
        std::unordered_map<DocId, double> merge_and(const std::unordered_map<DocId, double>& a, const std::unordered_map<DocId, double>& b) const; 
        // 详细说明：求两个文档集合的交集（AND操作）
        // 参数：
        //   a, b - 要求交集的文档得分映射
        // 返回值：交集文档的得分映射
		
        std::unordered_map<DocId, double> merge_not(const std::unordered_map<DocId, double>& a, const std::unordered_map<DocId, double>& b) const; 
        // 详细说明：从集合a中排除集合b的文档（NOT操作）
        // 参数：
        //   a - 原始文档集合
        //   b - 要排除的文档集合
        // 返回值：排除后的文档得分映射
		
        std::unordered_map<DocId, double> get_all_docs_universe() const; 
        // 详细说明：获取所有文档的集合，用于NOT操作的全集
        // 参数：无
        // 返回值：所有文档的得分映射（默认得分为0）
		
        bool is_phrase_token(const std::string& token) const; 
        // 详细说明：判断令牌是否为短语（用引号包围）
        // 参数：
        //   token - 查询令牌
        // 返回值：如果是短语返回true，否则false
		
        std::vector<std::string> extract_phrase_terms(const std::string& phrase_token) const; 
        // 详细说明：从短语令牌中提取词汇列表
        // 参数：
        //   phrase_token - 短语令牌（带引号的字符串）
        // 返回值：短语中的词汇列表

        InvertedIndex& index_;
        // 详细说明：倒排索引的引用，用于查询文档和获取统计信息


        Tokenizer query_tokenizer_;
        // 详细说明：查询分词器，专门用于处理查询字符串的分词
		
        bool default_use_bm25_ = false; 
        // 详细说明：默认排序算法标志，true表示默认使用BM25
        
    };
}


