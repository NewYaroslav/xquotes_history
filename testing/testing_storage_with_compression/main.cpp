#include "xquotes_files.hpp"
#include "xquotes_storage.hpp"
#include <vector>
#include <array>
#include <iostream>
#include <random>
#include <ctime>
#include <stdio.h>

#include <algorithm>
#include <random>

int main() {
    std::cout << "start!" << std::endl;
    std::string path = "D:\\_repoz\\finam_history_quotes\\xq4hex\\EURUSD";
    std::string path_dictionary = "..\\..\\dictionary\\candles.dat";
    std::string path_file = "test2.dat";
    std::vector<std::string> files_list;
    bf::get_list_files(path, files_list);

    xtime::timestamp_t t1, t2;
    xquotes_files::get_beg_end_timestamp(files_list, ".xq4hex", t1, t2);
    std::cout << xtime::get_str_date_time(t1) << " - " << xtime::get_str_date_time(t2) << std::endl;
    system("pause");

    int num_files = 0;
    if(bf::check_file(path_file)) {
        std::cout << "start check!" << std::endl;
        xquotes_storage::Storage iStorage(path_file, path_dictionary);

        for(size_t i = 0; i < files_list.size(); ++i) {
        int file_size = bf::get_file_size(files_list[i]);
            if(file_size > 0) {
                char *samples_buffer = new char[file_size];
                num_files++;
                int err = bf::load_file(files_list[i], samples_buffer, file_size, 0);
                if(err != file_size) {
                    std::cout << "load file error! file: " << files_list[i] << std::endl;
                } else {
                    std::cout << "check file: " << num_files << " - " << files_list.size() << " size " << file_size << std::endl;
                    char *test_buffer = NULL;
                    unsigned long test_buffer_size = 0;
                    int err_storage = iStorage.read_compressed_subfile(num_files, test_buffer, test_buffer_size);
                    if(err_storage != xquotes_storage::OK) std::cout << "storage error! error: " << err_storage << std::endl;
                    if(file_size != test_buffer_size) std::cout << "buffer error! test_buffer_size: " << test_buffer_size << std::endl;
                    else {
                        for(int i = 0; i < test_buffer_size; ++i) {
                            if(samples_buffer[i] != test_buffer[i]) {
                                std::cout << "buffer error! samples_buffer[i] != test_buffer[i] : " << samples_buffer[i] << " != " << test_buffer[i] << std::endl;
                            }
                        }
                    }
                    delete test_buffer;
                }
                delete samples_buffer;
            } else {
                std::cout << "buffer size: error, " << files_list[i] << std::endl;
            }
        }
        return 0;
    } else {
        xquotes_storage::Storage iStorage(path_file, path_dictionary);

        int num_files = 0;
        for(size_t i = 0; i < files_list.size(); ++i) {
            int file_size = bf::get_file_size(files_list[i]);
            if(file_size > 0) {
                char *samples_buffer = new char[file_size];
                num_files++;
                int err = bf::load_file(files_list[i], samples_buffer, file_size, 0);
                if(err != file_size) {
                    std::cout << "load file error! file: " << files_list[i] << std::endl;
                } else {
                    std::cout << "write file: " << files_list[i] << " #" << num_files << " - " << files_list.size() << std::endl;
                    int err_storage = iStorage.write_compressed_subfile(num_files, samples_buffer, file_size);
                    if(err_storage != xquotes_storage::OK) std::cout << "storage error! error: " << err_storage << std::endl;
                }
                delete samples_buffer;
            } else {
                std::cout << "buffer size: error, " << files_list[i] << std::endl;
            }
        }
    }
    return 0;
}

