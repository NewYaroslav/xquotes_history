#include <iostream>
#include "banana_filesystem.hpp"
#include <cctype>
#include <algorithm>

using namespace std;

std::string convert_hex_to_string(unsigned char value) {
    char hex_string[32] = {};
    sprintf(hex_string,"0x%.2X", value);
    return std::string(hex_string);
}

std::string str_toupper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                // static_cast<int(*)(int)>(std::toupper)         // wrong
                // [](int c){ return std::toupper(c); }           // wrong
                // [](char c){ return std::toupper(c); }          // wrong
                   [](unsigned char c){ return std::toupper(c); } // correct
                  );
    return s;
}

int main()
{
    cout << "start!" << endl;
    std::string path = "D://_repoz//finam_history_quotes//storage";
    std::vector<std::string> files_list;
    bf::get_list_files(path, files_list);

    // получаем имена валютных пар
    std::vector<std::string> symbol_list(files_list.size());
    for(size_t i = 0; i < files_list.size(); ++i) {
        std::vector<std::string> path_element_list;
        bf::parse_path(files_list[i], path_element_list);
        std::string file_name = path_element_list.back();
        std::size_t found_beg = file_name.find_first_of("_", 0);
        //std::size_t found_end = file_name.find_last_of("._", 0);
        symbol_list[i] = str_toupper(file_name.substr(found_beg + 1, 6));
        std::cout <<  files_list[i] << " " << symbol_list[i] << std::endl;
    }

    std::string list_symbol_out = "";
    for(size_t i = 0; i < files_list.size(); ++i) {
        list_symbol_out += symbol_list[i] + "\n";
    }
    bf::write_file("text.txt", (void*)list_symbol_out.c_str(), list_symbol_out.size());

    return 0;
}
