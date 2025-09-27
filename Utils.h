#pragma once
#include <iostream>
#include <algorithm>
#include <vector>
#include <cctype>
#include <locale>
#include <string>
using namespace std;


namespace jia
{   //×ª»»×Ö·û´®´óÐ¡Ð´
    static string to_lower(const string& s)
    {
        string result;
        result.reserve(s.size());
        for (auto c : s)
        {
            result.push_back(std::tolower(static_cast<unsigned char>(c)));
        }

        return result;
    }
}

