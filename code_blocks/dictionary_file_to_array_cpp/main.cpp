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
    std::string path = "..\\..\\dictionary\\currency_pair";
    std::string path2 = "..\\..\\dictionary\\currency_pair_hpp";
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
        symbol_list[i] = file_name.substr(found_beg + 1, 6);
        std::cout <<  files_list[i] << " " << symbol_list[i] << std::endl;
    }

    // создаем массивы
    for(size_t i = 0; i < files_list.size(); ++i) {
        std::string symbol = str_toupper(symbol_list[i]);

        std::string data;
        bf::load_file(files_list[i], data);
        std::string out;
        out += "#ifndef XQUOTES_DICTIONARY_" + symbol + "_HPP_INCLUDED\n";
        out += "#define XQUOTES_DICTIONARY_" + symbol + "_HPP_INCLUDED\n";
        out += "namespace xquotes_dictionary {\n";
        out += "\tconst int USE_DICTIONARY_" + symbol + " = " + std::to_string(i + 1) + ";\n";
        out += "\tconst static unsigned char dictionary_candles_" + symbol_list[i] + "[" + std::to_string(data.size()) + "] = {\n";
        out += "\t\t";
        for(size_t j = 0; j < data.size(); ++j) {
            if(j % 16 == 0) {
                out += "\n\t\t";
            }
            out += convert_hex_to_string((unsigned char)data[j]) + ",";
            if(j == data.size() - 1) {
                out += "\n\t};\n";
            }
        }
        out += "}\n";
        out += "#endif // XQUOTES_DICTIONARY_" + symbol + "_HPP_INCLUDED\n";

        std::string path_out = path2 + "\\xquotes_dictionary_candles_" + symbol_list[i] + ".hpp";
        bf::write_file(path_out, (void*)out.c_str(), out.size());
    }

    std::string list_symbol_out = "";
    for(size_t i = 0; i < files_list.size(); ++i) {
        std::string path_out = "#include \"dictionary_currency_pair/xquotes_dictionary_candles_" + symbol_list[i] + ".hpp\"";
        list_symbol_out += path_out + "\n";
    }
    bf::write_file(path2 + "\\text.txt", (void*)list_symbol_out.c_str(), list_symbol_out.size());
    return 0;

    std::string data;
    bf::load_file(path, data);
    std::string out;
    out += "const static char array[" + std::to_string(data.size()) + "] = {";

    for(size_t i = 0; i < data.size(); ++i) {
        if(i % 16 == 0) {
            out += "\n";
        }
        out += convert_hex_to_string((unsigned char)data[i]) + ",";
        if(i == data.size() - 1) {
            out += "\n";
        }
    }
    out += "};\n";
    std::string path_out = "..\\..\\dictionary\\candles_with_volumes.txt";
    bf::write_file(path_out, (void*)out.c_str(), out.size());
    return 0;
}
