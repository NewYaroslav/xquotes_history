//#include "xquotes_quotation_files.hpp"
#include "xquotes_csv.hpp"
#include "xquotes_zstd.hpp"
#include <vector>
#include <array>
#include <iostream>
#include <random>
#include <ctime>
#include <stdio.h>

#include <algorithm>
#include <random>

int main() {

    std::string path = "D:\\_repoz\\finam_history_quotes\\xq5hex";
    std::string path2 = "D:\\_repoz\\xquotes_history\\dukascopy\\xq5hex";
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
}

