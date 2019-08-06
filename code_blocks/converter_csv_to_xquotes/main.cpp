#include "xquotes_files.hpp"
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
int open_csv(
            std::string file_name,
            bool is_read_header,
            bool is_cet_to_gmt,
            std::function<void (
                    std::vector<xquotes_common::Candle> candles,
                    unsigned long long timestamp)> f);

int main(int argc, char *argv[]) {
    bool is_read_header = false;
    bool is_cet_to_gmt = false;
    std::string file_name = "";
    std::string path = "";

    const int ONLY_CLOSING_PRICE = 0;
    const int CANDLE_WITHOUT_VOLUME = 1;
    const int CANDLE_WITH_VOLUME = 2;
    int type = ONLY_CLOSING_PRICE;
    for(int i = 1; i < argc; ++i) {
        std::string value = std::string(argv[i]);
        if(value == "cet_to_gmt" || value == "-ctgt") is_cet_to_gmt = true;
        else
        if(value == "header" || value == "-h") is_read_header = true;
        else
        if((value == "path" || value == "-p") && i < argc) {
            path = std::string(argv[i + 1]);
        } else
        if(value == "csv") {
            file_name = std::string(argv[i + 1]);
        }
        if(value == "only_close" || value == "-oc") {
            type = ONLY_CLOSING_PRICE;
        }
        if(value == "candle" || value == "-c") {
            type = CANDLE_WITHOUT_VOLUME;
        }
        if(value == "candle_volume" || value == "-cv") {
            type = CANDLE_WITH_VOLUME;
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
    if(type == ONLY_CLOSING_PRICE) {
        std::string file_extension = ".xqhex";
        int err_csv = open_csv(
                    file_name,
                    is_read_header,
                    is_cet_to_gmt, [&](
                    std::array<double, xtime::MINUTES_IN_DAY> close,
                    unsigned long long timestamp) {
                xquotes_files::write_bin_file_u32_1x(
                path + "//" + xquotes_files::get_file_name_from_date(timestamp) + file_extension,
                close);
        });
        if(err_csv != xquotes_common::OK) std::cout << "error! code: " << err_csv << std::endl;
    } else
    if(type == CANDLE_WITHOUT_VOLUME) {
        std::string file_extension = ".xq4hex";
        int err_csv = open_csv(
                file_name,
                is_read_header,
                is_cet_to_gmt, [&](
                std::vector<xquotes_common::Candle> candles,
                unsigned long long timestamp) {
                xquotes_files::write_bin_file_u32_4x(
                path + "//" + xquotes_files::get_file_name_from_date(timestamp) + file_extension,
                candles);
        });
        if(err_csv != xquotes_common::OK) std::cout << "error! code: " << err_csv << std::endl;
    } else
    if(type == CANDLE_WITH_VOLUME) {
        std::string file_extension = ".xq5hex";
        int err_csv = open_csv(
                file_name,
                is_read_header,
                is_cet_to_gmt, [&](
                std::vector<xquotes_common::Candle> candles,
                unsigned long long timestamp) {
                xquotes_files::write_bin_file_u32_5x(
                path + "//" + xquotes_files::get_file_name_from_date(timestamp) + file_extension,
                candles);
        });
        if(err_csv != xquotes_common::OK) std::cout << "error! code: " << err_csv << std::endl;
    }
    return 0;
}

int open_csv(
            std::string file_name,
            bool is_read_header,
            bool is_cet_to_gmt,
            std::function<void (std::array<double, xtime::MINUTES_IN_DAY> close, unsigned long long timestamp)> f) {

    return xquotes_csv::read_file(
            file_name,
            is_read_header,
            is_cet_to_gmt,
            [&](xquotes_csv::Candle candle, bool is_end) {

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
            if(real_day != last_day || is_end) {
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

int open_csv(
            std::string file_name,
            bool is_read_header,
            bool is_cet_to_gmt,
            std::function<void (std::vector<xquotes_common::Candle> candles, unsigned long long timestamp)> f) {

    return xquotes_csv::read_file(
            file_name,
            is_read_header,
            is_cet_to_gmt,
            [&](xquotes_csv::Candle candle, bool is_end) {

        static int last_day = -1;
        static std::vector<xquotes_common::Candle> candles;
        static unsigned long long file_timestamp = 0;

        if(last_day == -1) { // момент инициализации
            last_day = xtime::get_day(candle.timestamp);
            file_timestamp = candle.timestamp;
            candles.push_back(candle);
        } else {
            int real_day = xtime::get_day(candle.timestamp);
            if(real_day != last_day || is_end) {
                last_day = real_day;
                // записываем данные тут
                xtime::DateTime iTime(file_timestamp);
                iTime.set_beg_day();
                file_timestamp = iTime.get_timestamp();
                std::cout << "write file: " << xtime::get_str_unix_date_time(file_timestamp) << std::endl;
                f(candles, file_timestamp);
                //...
                candles.clear();
                file_timestamp = candle.timestamp;
            }
            //candles[minute_day] = candle;
            candles.push_back(candle);
        }

    });
}
