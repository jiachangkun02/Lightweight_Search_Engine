#include "QueryProcessor.h"
#include "InvertedIndex.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <iomanip>
using namespace jia;
using namespace std;

// 用C++17的文件系统库，更方便地处理目录和文件操作
namespace fs = std::filesystem;

// 索引指定目录下所有txt文件的函数
// 1. 递归遍历目录下所有.txt文件
// 2. 读取文件内容并添加到倒排索引中
// 3. 显示处理进度和统计信息
void index_directory(InvertedIndex& index, const std::string& path) {
    // 详细说明：递归遍历指定目录，索引所有支持的文本文件
    // 功能：
    //   - 检查目录有效性
    //   - 递归遍历所有子目录
    //   - 过滤支持的文件类型（.txt, .md, .log）
    //   - 避免重复索引同一文件
    //   - 显示处理进度和统计信息
    // 参数：
    //   index - 倒排索引对象引用
    //   path - 要索引的目录路径
    // 返回值：无
    // 先检查目录是否存在且是一个有效目录
    if (!fs::exists(path) || !fs::is_directory(path)) {
        std::cerr << "错误：目录不存在或不是有效目录: " << path << std::endl;
        return;
    }

    // 遍历目录并统计处理的文件数量（保留旧实现供参考）
    // int count = 0;
    // for (const auto& entry : fs::recursive_directory_iterator(path)) {
    //     if (entry.is_regular_file() && entry.path().extension() == ".txt") {
    //         std::ifstream file(entry.path());
    //         if (file) {
    //             std::stringstream buffer;
    //             buffer << file.rdbuf();
    //             index.add_doc(entry.path().string(), buffer.str());
    //             count++;
    //             std::cout << "Indexed: " << entry.path().string() << std::endl;
    //         }
    //     }
    // }
    std::vector<std::string> exts = { ".txt",".md",".log" };
    int count = 0;
    int skipped = 0;
    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        std::string ext = entry.path().extension().string();
        if (std::find(exts.begin(), exts.end(), ext) == exts.end()) {
            continue;
        }
        std::string full_path = entry.path().string();
        if (index.has_doc(full_path)) {
            skipped++;
            continue;
        }
        std::ifstream file(entry.path(), std::ios::binary);
        if (file) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            index.add_doc(full_path, buffer.str());
            count++;
            std::cout << "已索引: " << full_path << std::endl;
        } else {
            std::cerr << "跳过（无法读取）: " << full_path << std::endl;
        }
    }
    std::cout << "\n索引完成，共索引 " << count << " 个文件，跳过已存在的 " << skipped << " 个。\n"
        << "当前索引文档总数: " << index.get_doc_count() << std::endl;
}

// 显示命令行帮助信息 - 列出所有支持的命令和简单用法说明
void print_help() {
    // 详细说明：显示程序的命令行界面帮助信息
    // 功能：向用户展示所有可用的命令及其用法
    // 参数：无
    // 返回值：无
    std::cout << "\n--- 轻量级搜索引擎 CLI ---\n"
        << "命令列表:\n"
        << "  index <目录>          : 递归索引目录下的 .txt/.md/.log 文件\n"
        << "  reindex <目录>        : 清空当前索引后重新索引目录\n"
        << "  query [选项] <查询>   : 查询，支持 AND/OR/NOT、括号、引号短语\n"
        << "     选项: -bm25 | -tfidf, -k <topN>, -offset <起始>\n"
        << "     示例: query -bm25 -k 20 \"information retrieval\" AND C++ NOT beginner\n"
        << "  save <文件>           : 保存索引（boost 格式）\n"
        << "  load <文件>           : 加载索引（boost 格式）\n"
        << "  savec <文件> / loadc  : 保存/加载紧凑索引\n"
        << "  stats                 : 查看索引统计\n"
        << "  help                  : 查看帮助\n"
        << "  exit                  : 退出程序\n"
        << "-------------------------------------\n";
}

// 主函数 - 实现交互式命令行界面
// 支持的命令：
// 1. index: 建立索引
// 2. query: 执行搜索
// 3. save/load: 保存/加载索引
// 4. stats: 显示统计信息
// 5. help: 显示帮助
// 6. exit: 退出程序
int main()
{
    // 详细说明：程序的主入口点，实现交互式命令行界面
    // 功能：
    //   - 初始化核心组件
    //   - 显示欢迎信息和帮助
    //   - 处理用户输入的命令
    //   - 提供完整的搜索引擎功能
    // 参数：无
    // 返回值：程序退出状态码

    // 创建核心对象
    InvertedIndex index;          // 倒排索引对象
    QueryProcessor qp(index);     // 查询处理器，关联到索引
    std::string line;            // 用户输入的命令行

    // 启动时显示帮助信息
    print_help();

    // 命令行交互循环
    while (true) {
        // 详细说明：主交互循环，持续接收和处理用户命令
        // 循环直到用户输入"exit"命令或遇到输入错误
        std::cout << "\n>> ";
        if (!std::getline(std::cin, line)) {
            break;
        }

        // 解析命令
        std::istringstream iss(line);
        std::string command;
        iss >> command;

        // 处理各种命令
        if (command == "exit") {
            break;
        }
        else if (command == "help") {
            print_help();
        }
        else if (command == "index") {
            std::string dir_path;
            if (iss >> dir_path) {
                index_directory(index, dir_path);
            }
            else {
                std::cerr << "用法: index <目录路径>" << std::endl;
            }
        }
        else if (command == "reindex") {
            std::string dir_path;
            if (iss >> dir_path) {
                index.clear();
                index_directory(index, dir_path);
            }
            else {
                std::cerr << "用法: reindex <目录路径>" << std::endl;
            }
        }
        else if (command == "save") {
            std::string file_path;
            if (iss >> file_path) {
                if (index.save_boost(file_path)) {
                    std::cout << "索引已保存到 " << file_path << std::endl;
                }
                else {
                    std::cerr << "错误：保存索引失败。" << std::endl;
                }
            }
            else {
                std::cerr << "用法: save <文件路径>" << std::endl;
            }
        }
        else if (command == "load") {
            std::string file_path;
            if (iss >> file_path) {
                if (index.load_boost(file_path)) {
                    std::cout << "已加载索引: " << file_path << std::endl;
                }
                else {
                    std::cerr << "错误：加载索引失败，文件不存在或已损坏。" << std::endl;
                }
            }
            else {
                std::cerr << "用法: load <文件路径>" << std::endl;
            }
        }
        else if (command == "savec") {
            std::string file_path;
            if (iss >> file_path) {
                if (index.save_compact(file_path)) {
                    std::cout << "紧凑索引已保存到 " << file_path << std::endl;
                } else {
                    std::cerr << "错误：保存紧凑索引失败。" << std::endl;
                }
            } else {
                std::cerr << "用法: savec <文件路径>" << std::endl;
            }
        }
        else if (command == "loadc") {
            std::string file_path;
            if (iss >> file_path) {
                if (index.load_compact(file_path)) {
                    std::cout << "已加载紧凑索引: " << file_path << std::endl;
                } else {
                    std::cerr << "错误：加载紧凑索引失败。" << std::endl;
                }
            } else {
                std::cerr << "用法: loadc <文件路径>" << std::endl;
            }
        }
        else if (command == "stats") {
            std::cout << "索引统计:\n"
                << " - 文档总数: " << index.get_doc_count() << std::endl
                << " - 词条总数: " << index.get_total_terms() << std::endl
                << " - 不重复词条数: " << index.get_unique_term_count() << std::endl
                << " - 平均文档长度: " << std::fixed << std::setprecision(2) << index.get_avg_doc_length() << std::endl;
        }
        else if (command == "query") {
            // 支持 -bm25/-tfidf、-k、-offset
            QueryProcessor::QueryOptions options;
            options.top_k = 10;
            options.offset = 0;
            std::string token;
            std::vector<std::string> query_tokens;
            while (iss >> token) {
                if (token == "-bm25") {
                    options.use_bm25 = true;
                }
                else if (token == "-tfidf") {
                    options.use_bm25 = false;
                }
                else if (token == "-k") {
                    size_t k = 0;
                    if (iss >> k) {
                        options.top_k = k;
                    }
                }
                else if (token == "-offset" || token == "-skip") {
                    size_t off = 0;
                    if (iss >> off) {
                        options.offset = off;
                    }
                }
                else {
                    query_tokens.push_back(token);
                }
            }
            if (query_tokens.empty()) {
                std::cerr << "用法: query [-bm25|-tfidf] [-k N] [-offset N] <查询语句>" << std::endl;
                continue;
            }
            std::string full_query;
            for (size_t i = 0; i < query_tokens.size(); ++i) {
                full_query += query_tokens[i];
                if (i + 1 < query_tokens.size()) {
                    full_query += " ";
                }
            }

            auto results = qp.process(full_query, options);
            std::cout << "共找到 " << results.size() << " 条结果（显示 top " << options.top_k << "，起始 offset=" << options.offset << "）。\n";
            int display_count = 0;
            for (const auto& res : results) {
                if (display_count++ >= static_cast<int>(options.top_k)) {
                    std::cout << "... (更多结果可调整 -k/-offset 查看)" << std::endl;
                    break;
                }
                std::cout << "  - [评分: " << std::fixed << std::setprecision(4) << res.score << "] "
                    << index.get_docINF(res.doc_id).doc_path << std::endl;
            }

        }
        else if (!command.empty()) {
            std::cerr << "未知命令: '" << command << "'。输入 help 查看可用命令。" << std::endl;
        }
    }


    return 0;
}
