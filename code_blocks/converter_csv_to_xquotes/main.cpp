#include "xquotes_files.hpp"
#include "xquotes_csv.hpp"
#include "xquotes_history.hpp"
#include <vector>
#include <array>
#include <iostream>
#include <random>
#include <ctime>
#include <stdio.h>

int open_csv(
            std::string file_name,
            bool is_read_header,
            int time_zone,
            int type_price,
            std::function<void (
                    std::array<double, xtime::MINUTES_IN_DAY> close,
                    xtime::timestamp_t timestamp)> f);

// для работы с данными свечей
int open_csv(
            std::string file_name,
            bool is_read_header,
            int time_zone,
            int type_price,
            std::function<void (
                    std::vector<xquotes_common::Candle> candles,
                    xtime::timestamp_t timestamp)> f);

int main(int argc, char *argv[]) {
    bool is_read_header = false;
    int time_zone = -1;
    int type_price = xquotes_history::PRICE_CLOSE;
    bool is_hex = false;
    bool is_compression = false;

    // задела на будущее для слияния файлов, теперь у нас массивы csv файлов и путей
    std::vector<std::string> paths;
    std::vector<std::string> files_name;

    // парисм параметры
    for(int i = 1; i < argc; ++i) {
        std::string value = std::string(argv[i]);
        if(value == "cet_to_gmt" || value == "-cetgmt" || value == "-finam") time_zone = xquotes_history::CET_TO_GMT;
        else
        if(value == "eet_to_gmt" || value == "-eetgmt" || value == "-alpari") time_zone = xquotes_history::EET_TO_GMT;
        else
        if(value == "gmt_to_cet" || value == "-cetgmt") time_zone = xquotes_history::GMT_TO_CET;
        else
        if(value == "gmt_to_eet" || value == "-eetgmt") time_zone = xquotes_history::GMT_TO_EET;
        else
        if(value == "header" || value == "-h") is_read_header = true;
        else
        if((value == "path" || value == "-p" || value == "-path") && i < argc) {
            paths.push_back(std::string(argv[i + 1]));
        } else
        if((value == "csv" || value == "-csv" || value == "path_csv" || value == "-pcsv") && i < argc) {
            files_name.push_back(std::string(argv[i + 1]));
        } else
        if(value == "only_close" || value == "-oc" || value == "-only_close") {
            type_price = xquotes_history::PRICE_CLOSE;
        } else
        if(value == "only_open" || value == "-oo" || value == "-only_open" ) {
            type_price = xquotes_history::PRICE_OPEN;
        } else
        if(value == "candle" || value == "-ohlc" || value == "-candle") {
            type_price = xquotes_history::PRICE_OHLC;
        } else
        if(value == "candle_volume" || value == "-ohlcv") {
            type_price = xquotes_history::PRICE_OHLCV;
        } else
        if(value == "hex" || value == "-hex") {
            is_hex = true;
        } else
        if(value == "compression" || value == "-c") {
            is_compression = true;
        } else
        if(value == "storage" || value == "-s") {
            is_hex = false;
        }
    }

    if(files_name.size() == 0 || paths.size() == 0) {
        std::cout << "parameter error! argc: " << argc << std::endl;
        for(int i = 0; i < argc; ++i) {
            std::cout << std::string(argv[i]) << std::endl;
        }
        std::cin;
        return 0;
    }

    if(is_hex && files_name.size() == 1 && paths.size() == 1) {
        if(type_price == xquotes_history::PRICE_CLOSE || type_price == xquotes_history::PRICE_OPEN) {
            std::string file_extension = ".xqhex";
            int err_csv = open_csv(
                        files_name[0],
                        is_read_header,
                        time_zone,
                        type_price,
                        [&](
                            std::array<double, xtime::MINUTES_IN_DAY> close,
                            xtime::timestamp_t timestamp) {
                xquotes_files::write_bin_file_u32_1x(
                paths[0] + "//" + xquotes_files::get_file_name_from_date(timestamp) + file_extension,
                close);
            });
            if(err_csv != xquotes_common::OK) std::cout << "error! code: " << err_csv << std::endl;
        } else
        if(type_price == xquotes_history::PRICE_OHLC || type_price == xquotes_history::PRICE_OHLCV) {
            std::string file_extension;
            if(type_price == xquotes_history::PRICE_OHLC) file_extension = ".xq4hex";
            else if(type_price == xquotes_history::PRICE_OHLCV) file_extension = ".xq5hex";
            int err_csv = open_csv(
                    files_name[0],
                    is_read_header,
                    time_zone,
                    type_price,
                    [&](
                        std::vector<xquotes_common::Candle> candles,
                        xtime::timestamp_t timestamp) {
                if(type_price == xquotes_history::PRICE_OHLC) {
                    xquotes_files::write_bin_file_u32_4x(
                        paths[0] + "//" + xquotes_files::get_file_name_from_date(timestamp) + file_extension,
                        candles);
                } else
                if(type_price == xquotes_history::PRICE_OHLCV) {
                    xquotes_files::write_bin_file_u32_5x(
                        paths[0] + "//" + xquotes_files::get_file_name_from_date(timestamp) + file_extension,
                        candles);
                }
            });
            if(err_csv != xquotes_common::OK) std::cout << "error! code: " << err_csv << std::endl;
        }
    } else
    if(files_name.size() == 1 && paths.size() == 1) {
        std::string path = paths[0];
        if(type_price == xquotes_history::PRICE_CLOSE ||
            type_price == xquotes_history::PRICE_OPEN) path += ".qhs";
        if(type_price == xquotes_history::PRICE_OHLC) path += ".qhs4";
        if(type_price == xquotes_history::PRICE_OHLCV) path += ".qhs5";

        int option = is_compression ? xquotes_history::USE_COMPRESSION : xquotes_history::DO_NOT_USE_COMPRESSION;
        xquotes_history::QuotesHistory<> iQuotesHistory(path, type_price, option);

        int err_csv = open_csv(
                files_name[0],
                is_read_header,
                time_zone,
                type_price,
                [&](
                    std::vector<xquotes_common::Candle> candles,
                    xtime::timestamp_t timestamp) {
            std::array<xquotes_common::Candle, xquotes_common::MINUTES_IN_DAY> new_candles;
            for(size_t i = 0; i < candles.size(); ++i) {
                new_candles[xtime::get_minute_day(candles[i].timestamp)] = candles[i];
            }
            iQuotesHistory.write_candles(new_candles, timestamp);
            //system("pause");
        });
        if(err_csv != xquotes_common::OK) std::cout << "error! code: " << err_csv << std::endl;
    }
    return 0;
}

int open_csv(
            std::string file_name,
            bool is_read_header,
            int time_zone,
            int type_price,
            std::function<void (std::array<double, xtime::MINUTES_IN_DAY> price, xtime::timestamp_t timestamp)> f) {

    return xquotes_csv::read_file(
            file_name,
            is_read_header,
            time_zone,
            [&](xquotes_csv::Candle candle, bool is_end) {

        static int last_day = -1;
        static std::array<double, xtime::MINUTES_IN_DAY> price;
        static xtime::timestamp_t file_timestamp = 0;
        int minute_day = xtime::get_minute_day(candle.timestamp);
        if(last_day == -1) { // момент инициализации
            last_day = xtime::get_day(candle.timestamp);
            file_timestamp = candle.timestamp;
            std::fill(price.begin(), price.end(), 0);
            price[minute_day] = type_price == xquotes_common::PRICE_CLOSE ?
                candle.close :
                type_price == xquotes_common::PRICE_OPEN ? candle.open :
                type_price == xquotes_common::PRICE_LOW ? candle.low :
                type_price == xquotes_common::PRICE_HIGH ? candle.high :
                candle.close;
        } else {
            int real_day = xtime::get_day(candle.timestamp);
            if(real_day != last_day || is_end) {
                last_day = real_day;
                // записываем данные тут
                xtime::DateTime iTime(file_timestamp);
                iTime.set_beg_day();
                file_timestamp = iTime.get_timestamp();
                std::cout << "write file: " << xtime::get_str_date(file_timestamp) << std::endl;
                f(price, file_timestamp);
                //...
                std::fill(price.begin(), price.end(), 0);
                file_timestamp = candle.timestamp;
            }
            price[minute_day] = candle.close;
        }

    });
}

int open_csv(
            std::string file_name,
            bool is_read_header,
            int time_zone,
            int type_price,
            std::function<void (std::vector<xquotes_common::Candle> candles, xtime::timestamp_t timestamp)> f) {

    return xquotes_csv::read_file(
            file_name,
            is_read_header,
            time_zone,
            [&](xquotes_csv::Candle candle, bool is_end) {

        static int last_day = -1;
        static std::vector<xquotes_common::Candle> candles;
        static xtime::timestamp_t file_timestamp = 0;

        if(last_day == -1) { // момент инициализации
            last_day = xtime::get_day(candle.timestamp);
            file_timestamp = xtime::get_start_day(candle.timestamp);
            candles.push_back(candle);
        } else {
            int real_day = xtime::get_day(candle.timestamp);
            if(real_day != last_day || is_end) {
                last_day = real_day;
                // записываем данные тут
                std::cout << "write file: " << xtime::get_str_date(file_timestamp) << std::endl;
                f(candles, file_timestamp);
                //...
                candles.clear();
                file_timestamp = xtime::get_start_day(candle.timestamp);
            }
            candles.push_back(candle);
        }

    });
}
