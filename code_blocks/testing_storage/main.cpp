#include "xquotes_storage.hpp"
//#include "xquotes_csv.hpp"
#include <vector>
#include <array>
#include <iostream>
#include <random>
#include <ctime>
#include <stdio.h>

void write_test_data(xquotes_storage::Storage &iStorage, unsigned long long key, int size) {
    char *test_data = new char[size];
    for(int i = 0; i < size; ++i) test_data[i] = i;
    int err = iStorage.write_subfile(key, test_data, size);
    std::cout << "write_subfile " << key << " code " << err << std::endl;
    delete[] test_data;
    std::cout << "!" << std::endl;
}

void check_test_data(xquotes_storage::Storage &iStorage, unsigned long long key, int size) {
    char *data = NULL;
    unsigned long len = 0;
    int err = iStorage.get_subfile_size(key, len);
    std::cout << "get_subfile_size " << key << " code " << err << " len " << len << std::endl;
    err = iStorage.read_subfile(key, data, len);
    std::cout << "read_subfile " << key << " code " << err << " len " << len << std::endl;
    if(err == 0) {
        bool is_err = false;
        for(int i = 0; i < len; ++i) {
            if(i != data[i]) {
                std::cout << "error cmp " << i << " " << data[i] <<  std::endl;
                is_err = true;
                break;
            }
        }
        if(!is_err)  std::cout << "cmp ok!" << std::endl;
    }
    delete data;
}

int main(int argc, char *argv[]) {
    std::cout << "start!" << std::endl;

    xquotes_storage::Storage iStorage("test.dat");

    unsigned long long key0 = 0x0; unsigned long size0 = 8;
    unsigned long long key1 = 0x1; unsigned long size1 = 16;
    unsigned long long key2 = 0x2; unsigned long size2 = 32;
    unsigned long long key3 = 0x3; unsigned long size3 = 64;

    std::cout << "step 1" << std::endl;
    check_test_data(iStorage, key0, size0);
    write_test_data(iStorage, key0, size0);
    check_test_data(iStorage, key0, size0);
    std::cout << "step 2" << std::endl;
    write_test_data(iStorage, key1, size1);
    check_test_data(iStorage, key1, size1);
    std::cout << "step 3" << std::endl;
    write_test_data(iStorage, key2, size2);
    check_test_data(iStorage, key2, size2);
    std::cout << "step 4" << std::endl;
    write_test_data(iStorage, key0, size2);
    check_test_data(iStorage, key0, size2);
    std::cout << "step 5" << std::endl;
    check_test_data(iStorage, key1, size1);
    system("pause");
    return 0;
}


