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

// ��C++17���ļ�ϵͳ�⣬������ش���Ŀ¼���ļ�����
namespace fs = std::filesystem;

// ����ָ��Ŀ¼������txt�ļ��ĺ���
// 1. �ݹ����Ŀ¼������.txt�ļ�
// 2. ��ȡ�ļ����ݲ���ӵ�����������
// 3. ��ʾ������Ⱥ�ͳ����Ϣ
void index_directory(InvertedIndex& index, const std::string& path) {
    // �ȼ��Ŀ¼�Ƿ��������һ����ЧĿ¼
    if (!fs::exists(path) || !fs::is_directory(path)) {
        std::cerr << "Error: Directory not found or not a directory: " << path << std::endl;
        return;
    }

    // ����Ŀ¼��ͳ�ƴ�����ļ�����
    int count = 0;
    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            std::ifstream file(entry.path());
            if (file) {
                // ��stringstreamһ���Զ�ȡ�����ļ�����
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

// ��ʾ�����а�����Ϣ - �г�����֧�ֵ�����ͼ��÷�˵��
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

// ������ - ʵ�ֽ���ʽ�����н���
// ֧�ֵ����
// 1. index: ��������
// 2. query: ִ������
// 3. save/load: ����/��������
// 4. stats: ��ʾͳ����Ϣ
// 5. help: ��ʾ����
// 6. exit: �˳�����
int main()
{
    // �������Ķ���
    InvertedIndex index;          // ������������
    QueryProcessor qp(index);     // ��ѯ������������������
    std::string line;            // �û������������

    // ����ʱ��ʾ������Ϣ
    print_help();

    // �����н���ѭ��
    while (true) {
        std::cout << "\n>> ";
        if (!std::getline(std::cin, line)) {
            break;
        }

        // ��������
        std::istringstream iss(line);
        std::string command;
        iss >> command;

        // �����������
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
                if (display_count++ >= 10) { // ֻ��ʾǰ10�����
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