#include "QueryProcessor.h"
#include "InvertedIndex.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
using namespace jia;
using namespace std;

// 用C++17的文件系统库，更方便地处理目录和文件操作
namespace fs = std::filesystem;

// 索引指定目录下所有txt文件的函数
// 1. 递归遍历目录下所有.txt文件
// 2. 读取文件内容并添加到倒排索引中
// 3. 显示处理进度和统计信息
void index_directory(InvertedIndex& index, const std::string& path) {
    // 先检查目录是否存在且是一个有效目录
    if (!fs::exists(path) || !fs::is_directory(path)) {
        std::cerr << "Error: Directory not found or not a directory: " << path << std::endl;
        return;
    }

    // 遍历目录并统计处理的文件数量
    int count = 0;
    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            std::ifstream file(entry.path());
            if (file) {
                // 用stringstream一次性读取整个文件内容
                std::stringstream buffer;
                buffer << file.rdbuf();
                index.add_doc(entry.path().string(), buffer.str());
                count++;
                std::cout << "Indexed: " << entry.path().string() << std::endl;
            }
        }
    }
    std::cout << "\nFinished indexing. " << count << " files indexed.\n"
        << "Total documents in index: " << index.get_doc_count() << std::endl;
}

// 显示命令行帮助信息 - 列出所有支持的命令和简单用法说明
void print_help() {
    std::cout << "\n--- Lightweight Search Engine CLI ---\n"
        << "Commands:\n"
        << "  index <dir>      : Index all .txt files in a directory.\n"
        << "  query <terms>    : Search for terms. Supports AND, NOT.\n"
        << "                     Example: query C++ AND language NOT beginner\n"
        << "  save <filepath>  : Save the current index to a file.\n"
        << "  load <filepath>  : Load an index from a file.\n"
        << "  stats            : Show statistics about the current index.\n"
        << "  help             : Show this help message.\n"
        << "  exit             : Exit the program.\n"
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
    // 创建核心对象
    InvertedIndex index;          // 倒排索引对象
    QueryProcessor qp(index);     // 查询处理器，关联到索引
    std::string line;            // 用户输入的命令行

    // 启动时显示帮助信息
    print_help();

    // 命令行交互循环
    while (true) {
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
                std::cerr << "Usage: index <directory_path>" << std::endl;
            }
        }
        else if (command == "save") {
            std::string file_path;
            if (iss >> file_path) {
                if (index.save_boost(file_path)) {
                    std::cout << "Index saved successfully to " << file_path << std::endl;
                }
                else {
                    std::cerr << "Error: Failed to save index." << std::endl;
                }
            }
            else {
                std::cerr << "Usage: save <filepath>" << std::endl;
            }
        }
        else if (command == "load") {
            std::string file_path;
            if (iss >> file_path) {
                if (index.load_boost(file_path)) {
                    std::cout << "Index loaded successfully from " << file_path << std::endl;
                }
                else {
                    std::cerr << "Error: Failed to load index. File might not exist or is corrupted." << std::endl;
                }
            }
            else {
                std::cerr << "Usage: load <filepath>" << std::endl;
            }
        }
        else if (command == "stats") {
            std::cout << "Index Statistics:\n"
                << " - Total documents: " << index.get_doc_count() << std::endl;
        }
        else if (command == "query") {
            std::string query_part;
            std::string full_query;
            while (iss >> query_part) {
                full_query += query_part + " ";
            }
            if (full_query.empty()) {
                std::cerr << "Usage: query <search terms>" << std::endl;
                continue;
            }

            auto results = qp.process(full_query);
            std::cout << "Found " << results.size() << " results.\n";
            int display_count = 0;
            for (const auto& res : results) {
                if (display_count++ >= 10) { // 只显示前10个结果
                    std::cout << "... (and " << results.size() - 10 << " more)" << std::endl;
                    break;
                }
                std::cout << "  - [Score: " << std::fixed << std::setprecision(4) << res.score << "] "
                    << index.get_docINF(res.doc_id).doc_path << std::endl;
            }

        }
        else if (!command.empty()) {
            std::cerr << "Unknown command: '" << command << "'. Type 'help' for a list of commands." << std::endl;
        }
    }


    return 0;
}