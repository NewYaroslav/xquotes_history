#include "xquotes_files.hpp"
#include "xquotes_csv.hpp"
#include "xquotes_history.hpp"
#include <vector>
#include <array>
#include <iostream>
#include <random>
#include <ctime>
#include <stdio.h>

int main(int argc, char *argv[]) {
    std::string file_name_csv_mt4 = "mt4_example.csv";
    std::string file_name_csv_mt5 = "mt5_example.csv";
    std::string file_name_csv_dukascopy = "dukascopy_example.csv";

    std::cout << file_name_csv_mt4 << std::endl;
    int err_mt4 = xquotes_csv::read_file(
            file_name_csv_mt4,
            false,
            xquotes_csv::DO_NOT_CHANGE_TIME_ZONE,
            [&](const xquotes_csv::Candle candle, const bool is_end) {
        std::cout
            << "candle: "
            << xtime::get_str_date_time(candle.timestamp) << " "
            << candle.open << " "
            << candle.high << " "
            << candle.low << " "
            << candle.close << " "
            << candle.volume << " is end "
            << is_end
            << std::endl;
    });
    std::cout << "err: " << err_mt4 << std::endl;

    std::cout << file_name_csv_mt5 << std::endl;
    int err_mt5 = xquotes_csv::read_file(
            file_name_csv_mt5,
            false,
            xquotes_csv::DO_NOT_CHANGE_TIME_ZONE,
            [&](const xquotes_csv::Candle candle, const bool is_end) {
        std::cout
            << "candle: "
            << xtime::get_str_date_time(candle.timestamp) << " "
            << candle.open << " "
            << candle.high << " "
            << candle.low << " "
            << candle.close << " "
            << candle.volume << " is end "
            << is_end
            << std::endl;
    });
    std::cout << "err: " << err_mt5 << std::endl;

    std::cout << file_name_csv_dukascopy << std::endl;
    int err_dukascopy = xquotes_csv::read_file(
            file_name_csv_dukascopy,
            false,
            xquotes_csv::DO_NOT_CHANGE_TIME_ZONE,
            [&](const xquotes_csv::Candle candle, const bool is_end) {
        std::cout
            << "candle: "
            << xtime::get_str_date_time(candle.timestamp) << " "
            << candle.open << " "
            << candle.high << " "
            << candle.low << " "
            << candle.close << " "
            << candle.volume << " is end "
            << is_end
            << std::endl;
    });
    std::cout << "err: " << err_dukascopy << std::endl;

    std::cout << "write: test_mt4_example.csv" << std::endl;
    xquotes_csv::write_file(
        "test_mt4_example.csv",
        "",
        false,
        xtime::get_timestamp(1,1,2019,0,0,0),
        xtime::get_timestamp(1,1,2019,23,59,0),
        xquotes_csv::MT4,
        xquotes_csv::SKIPPING_BAD_CANDLES,
        xquotes_csv::DO_NOT_CHANGE_TIME_ZONE,
        5,
        [&](xquotes_csv::Candle &candle, const xtime::timestamp_t timestamp) -> bool {
            candle.open = 64.5;
            candle.high = 64.5;
            candle.low = 64.5;
            candle.close = 64.5;
            candle.volume = 10.1;
            candle.timestamp = timestamp;
            return true;
    });


    std::cout << "write: test_mt5_example.csv" << std::endl;
    xquotes_csv::write_file(
        "test_mt5_example.csv",
        "<DATE>\t<TIME>\t<OPEN>\t<HIGH>\t<LOW>\t<CLOSE>\t<TICKVOL>\t<VOL>\t<SPREAD>",
        true,
        xtime::get_timestamp(1,1,2019,0,0,0),
        xtime::get_timestamp(1,1,2019,23,59,0),
        xquotes_csv::MT5,
        xquotes_csv::SKIPPING_BAD_CANDLES,
        xquotes_csv::DO_NOT_CHANGE_TIME_ZONE,
        5,
        [&](xquotes_csv::Candle &candle, const xtime::timestamp_t timestamp) -> bool {
            candle.open = 64.5;
            candle.high = 64.5;
            candle.low = 64.5;
            candle.close = 64.5;
            candle.volume = 10.1;
            candle.timestamp = timestamp;
            return true;
    });

    std::cout << "write: test_dukascopy_example.csv" << std::endl;
    xquotes_csv::write_file(
        "test_dukascopy_example.csv",
        "Gmt time,Open,High,Low,Close,Volume",
        true,
        xtime::get_timestamp(1,1,2019,0,0,0),
        xtime::get_timestamp(1,1,2019,23,59,0),
        xquotes_csv::DUKASCOPY,
        xquotes_csv::SKIPPING_BAD_CANDLES,
        xquotes_csv::DO_NOT_CHANGE_TIME_ZONE,
        5,
        [&](xquotes_csv::Candle &candle, const xtime::timestamp_t timestamp) -> bool {
            candle.open = 64.5;
            candle.high = 64.5;
            candle.low = 64.5;
            candle.close = 64.5;
            candle.volume = 10.1;
            candle.timestamp = timestamp;
            return true;
    });
    return 0;
}
