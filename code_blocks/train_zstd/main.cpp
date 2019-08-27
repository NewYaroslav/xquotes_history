//#include "xquotes_quotation_files.hpp"
#include "xquotes_csv.hpp"
#include "xquotes_zstd.hpp"
#include <vector>
#include <array>
#include <iostream>
#include <random>
#include <ctime>
#include <stdio.h>

#include <cctype>
#include <algorithm>
#include <random>

int main() {

    std::string path = "D:\\_repoz\\finam_history_quotes\\xq4hex";
    std::string path2 = "D:\\_repoz\\xquotes_history\\dukascopy\\xq4hex";
    std::vector<std::string> dir_list;
    bf::get_list_files(path, dir_list, true, true);

    std::vector<std::string> symbol_list(dir_list.size());
    for(size_t i = 0; i < dir_list.size(); ++i) {
        std::vector<std::string> path_element_list;
        bf::parse_path(dir_list[i], path_element_list);
        symbol_list[i] = path_element_list.back();
        std::cout << dir_list[i] << " " << symbol_list[i] << std::endl;
    }

    // обучаем каждую вещь по отдельности
    for(size_t i = 0; i < symbol_list.size(); ++i) {
        std::vector<std::string> files_list;
        bf::get_list_files(dir_list[i], files_list);
        auto rng = std::default_random_engine {};
        std::shuffle(files_list.begin(), files_list.end(), rng);
        files_list.resize(files_list.size()/2);

        std::string symbol = symbol_list[i];
        std::transform(symbol.begin(), symbol.end(), symbol.begin(), tolower);
        std::string file_name = "..\\..\\dictionary\\currency_pair\\candles_" + symbol + ".dat";
        std::cout << "dictionary: " << file_name << std::endl;
        xquotes_zstd::train_zstd(files_list, file_name.c_str());
    }
    return 0;
#   ifdef OLD_VERSION
    std::string path = "D:\\_repoz\\finam_history_quotes\\xq4hex";
    std::string path2 = "D:\\_repoz\\xquotes_history\\dukascopy\\xq4hex";
    std::vector<std::string> files_list;
    std::vector<std::string> files_list2;
    bf::get_list_files(path, files_list);
    bf::get_list_files(path2, files_list2);
    auto rng = std::default_random_engine {};
    std::shuffle(files_list.begin(), files_list.end(), rng);
    std::shuffle(files_list2.begin(), files_list2.end(), rng);

    const int SAMPLES = 25000;
    const int SAMPLES2 = 800;
    files_list.resize(SAMPLES);
    files_list2.resize(SAMPLES2);
    files_list.insert(files_list.end(), files_list2.begin(), files_list2.end());

    xquotes_zstd::train_zstd(files_list, "candles_with_volume.dat");
    return 0;
#   endif
}

