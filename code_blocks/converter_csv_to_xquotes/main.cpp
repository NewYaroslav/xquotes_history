#include "xquotes_quotation_files.hpp"
#include "xquotes_csv.hpp"
#include <vector>
#include <array>
#include <iostream>
#include <random>
#include <ctime>
#include <stdio.h>

int open_csv(
            std::string file_name,
            bool is_read_header,
            bool is_cet_to_gmt,
            std::function<void (
                    std::array<double, xtime::MINUTES_IN_DAY> close,
                    unsigned long long timestamp)> f);

int main(int argc, char *argv[]) {
    bool is_read_header = false;
    bool is_cet_to_gmt = false;
    std::string file_name = "";
    std::string path = "";
    std::string file_extension = ".xqhex";
    for(int i = 1; i < argc; ++i) {
        std::string value = std::string(argv[i]);
        if(value == "cet_to_gmt") is_cet_to_gmt = true;
        else
        if(value == "header") is_read_header = true;
        else
        if(value == "path" && i < argc) {
            path = std::string(argv[i + 1]);
        } else
        if(value == "csv") {
            file_name = std::string(argv[i + 1]);
        }
    }
    if(file_name == "" || path == "") {
        std::cout << "parameter error! argc: " << argc << std::endl;
        for(int i = 0; i < argc; ++i) {
            std::cout << std::string(argv[i]) << std::endl;
        }
        std::cin;
        return 0;
    }
    int err_csv = open_csv(
            file_name,
            is_read_header,
            is_cet_to_gmt, [&](
            std::array<double, xtime::MINUTES_IN_DAY> close,
            unsigned long long timestamp) {
        xquotes_history::write_binary_file(
            path + "//" + xquotes_history::get_file_name_from_date(timestamp) + file_extension,
            close);
    });

    return 0;
}

int open_csv(
            std::string file_name,
            bool is_read_header,
            bool is_cet_to_gmt,
            std::function<void (std::array<double, xtime::MINUTES_IN_DAY> close, unsigned long long timestamp)> f) {
    return xquotes_csv::read_file(file_name, is_read_header, is_cet_to_gmt, [&](xquotes_csv::Candle candle) {
        static int last_day = -1;
        static std::array<double, xtime::MINUTES_IN_DAY> close;
        static unsigned long long file_timestamp = 0;
        int minute_day = xtime::get_minute_day(candle.timestamp);
        if(last_day == -1) { // момент инициализации
            last_day = xtime::get_day(candle.timestamp);
            file_timestamp = candle.timestamp;
            std::fill(close.begin(), close.end(), 0);
            close[minute_day] = candle.close;
        } else {
            int real_day = xtime::get_day(candle.timestamp);
            if(real_day != last_day) {
                last_day = real_day;
                // записываем данные тут
                xtime::DateTime iTime(file_timestamp);
                iTime.set_beg_day();
                file_timestamp = iTime.get_timestamp();
                std::cout << "write file: " << xtime::get_str_unix_date_time(file_timestamp) << std::endl;
                f(close, file_timestamp);
                //...
                std::fill(close.begin(), close.end(), 0);
                file_timestamp = candle.timestamp;
            }
            close[minute_day] = candle.close;
        }

    });
}
