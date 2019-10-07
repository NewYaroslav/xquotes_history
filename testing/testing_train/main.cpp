#include "xquotes_files.hpp"
#include "xquotes_csv.hpp"
#include "xquotes_history.hpp"
#include "xquotes_zstd.hpp"
#include <vector>
#include <array>
#include <iostream>
#include <random>
#include <ctime>
#include <stdio.h>

#define ZQHTOOLS_VERSION "1.2"

enum {
    XQHTOOLS_CSV_TO_HEX = 0,
    XQHTOOLS_CSV_TO_QHS,
    XQHTOOLS_QHS_TO_CSV,
    XQHTOOLS_QHS_MERGE,
    XQHTOOLS_QHS_DATE,
    XQHTOOLS_ZSTD_TRAIN,
    XQHTOOLS_VERSION,
};

// получаем команду из командной строки
int get_cmd(int &cmd, const int argc, char *argv[]);
// конвертируем csv в hex фалы
int csv_to_hex(const int argc, char *argv[]);
// конвертируем csv в qhs файлы
int csv_to_qhs(const int argc, char *argv[]);
// конвертировать хранилище котировок в csv файл
int qhs_to_csv(const int argc, char *argv[]);
// получить дату
int qhs_date(const int argc, char *argv[]);
// получить дату
int qhs_date(const int argc, char *argv[]);
// обучение zstd
int zstd_train(const int argc, char *argv[]);
// слияние
int merge_date(const int argc, char *argv[]);
void parse(std::string value, std::vector<std::string> &elemet_list);

int main(int argc, char *argv[]) {
    return zstd_train(argc, argv);
}

int zstd_train(const int argc, char *argv[]) {
    std::string path_raw_storage = "../../storage/AUDCHF.qhs4";
    std::string path_raw;
    std::string path_dictionary = "test.hpp";
    std::string dictionary_name = "test";
    bool is_rnd = false;
    bool is_cpp = true;
    int fill_factor = 100;
    size_t dictionary_capacity = 102400;

    if(path_dictionary == "" || (dictionary_name == "" && is_cpp)) {
        std::cout << "error! no file name or file path specified!" << std::endl;
        return -1;
    }

    if(path_raw_storage != "") {
        xquotes_storage::Storage iStorage(path_raw_storage);

        // получим количество подфайлов
        int num_subfiles = iStorage.get_num_subfiles();
        if(num_subfiles == 0) {
            std::cout << "error! no subfiles!" << std::endl;
            return -1;
        }

        // сформируем список подфайлов
        int user_num_subfiles = num_subfiles; // новое количество подфайлов для обучения
        if(fill_factor < 100) {
            user_num_subfiles = user_num_subfiles * fill_factor / 100;
        }
        if(user_num_subfiles == 0) {
            std::cout << "error! no subfiles!" << std::endl;
            return -1;
        }

        std::vector<int> subfiles_list; // список подфайлов для обучения

        // настроим генератор случайных чисел
        std::mt19937 gen;
        gen.seed(time(0));
        std::uniform_int_distribution<> denominator(0, num_subfiles);

        for(int i = 0; i < user_num_subfiles; ++i) {
            if(is_rnd) {
                subfiles_list.push_back(iStorage.get_key_subfiles(denominator(gen)));
            } else {
                subfiles_list.push_back(iStorage.get_key_subfiles(i));
            }
        }

        bool is_file = is_cpp ? false : true;
        int err = xquotes_zstd::train_zstd(iStorage, subfiles_list, path_dictionary, dictionary_name, dictionary_capacity, is_file);
        if(err != xquotes_zstd::OK) {
            std::cout << "error!" << std::endl;
            return -1;
        }
    } else
    if(path_raw != "") {
        std::vector<std::string> files_list;
        bf::get_list_files(path_raw, files_list, true);
        if(files_list.size() == 0) {
            std::cout << "error! no files" << std::endl;
            return -1;
        }

        // найдем количество файлов для обучения
        int user_num_files = files_list.size();
        if(fill_factor < 100) {
            user_num_files = user_num_files * fill_factor / 100;
        }
        if(user_num_files == 0) {
            std::cout << "error! no files" << std::endl;
            return -1;
        }

        std::mt19937 gen;
        gen.seed(time(0));
        std::uniform_int_distribution<> denominator(0, files_list.size());

        std::vector<std::string> new_files_list;
        for(int i = 0; i < user_num_files; ++i) {
            if(is_rnd) {
                new_files_list.push_back(files_list[denominator(gen)]);
            } else {
                new_files_list.push_back(files_list[i]);
            }
        }

        bool is_file = is_cpp ? false : true;
        int err = xquotes_zstd::train_zstd(files_list, path_dictionary, dictionary_name, dictionary_capacity, is_file);
        if(err != xquotes_zstd::OK) {
            std::cout << "error!" << std::endl;
            return -1;
        }
    }
    return 0;
}

int merge_date(const int argc, char *argv[]) {
    std::vector<std::string> paths_raw_storages;
    std::string path_out_raw_storage = "";
    for(int i = 1; i < argc; ++i) {
        std::string value = std::string(argv[i]);
        if((value == "paths_raw_storages") && (i + 1) < argc) {
            std::string path_raw = std::string(argv[i + 1]);
            parse(path_raw, paths_raw_storages);
        } else
        if((value == "path_out_raw_storage") && (i + 1) < argc) {
            path_out_raw_storage = std::string(argv[i + 1]);
        }
    }

    if(path_out_raw_storage == "" || paths_raw_storages.size() <= 1) {
        std::cout << "error, not all data specified!" << std::endl;
        return -1;
    }

    std::vector<xquotes_storage::Storage *> list_storage;
    for(size_t i = 0; i < paths_raw_storages.size(); ++i) {
        list_storage.push_back(new xquotes_storage::Storage(paths_raw_storages[i]));
    }
    xquotes_storage::Storage OutStorage(path_out_raw_storage);
    std::cout << "start merge" << std::endl;
    int num_subfiles = 0;
    for(size_t i = 0; i < paths_raw_storages.size(); ++i) {
        int storage_subfiles = list_storage[i]->get_num_subfiles();
        std::cout << "storage " << paths_raw_storages[i] << " subfiles: " << storage_subfiles << std::endl;
        for(int s = 0; s < storage_subfiles; ++s) {
            xquotes_storage::key_t key = list_storage[i]->get_key_subfiles(s);
            char *buffer = NULL;
            unsigned long buffer_size = 0;
            int err = list_storage[i]->read_subfile(key, buffer, buffer_size);
            if(err != xquotes_storage::OK) {
                std::cout << "storage error, code " << err << std::endl;
                delete [] buffer;
                continue;
            }

            xquotes_storage::key_t new_key = num_subfiles;
            OutStorage.write_subfile(new_key, buffer, buffer_size);
            delete [] buffer;
            std::cout << "subfiles: " << num_subfiles << "\r";
            num_subfiles++;
        }
    }
    std::cout << std::endl;
    return 0;
}

void parse(std::string value, std::vector<std::string> &elemet_list) {
    if(value.back() != ',')
        value += ",";
    std::size_t start_pos = 0;
    while(true) {
        std::size_t found_beg = value.find_first_of(",", start_pos);
        if(found_beg != std::string::npos) {
            std::size_t len = found_beg - start_pos;
            if(len > 0)
                elemet_list.push_back(value.substr(start_pos, len));
            start_pos = found_beg + 1;
        } else break;
    }
}
