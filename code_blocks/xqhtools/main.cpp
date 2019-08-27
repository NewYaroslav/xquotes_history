#include "xquotes_files.hpp"
#include "xquotes_csv.hpp"
#include "xquotes_history.hpp"
#include <vector>
#include <array>
#include <iostream>
#include <random>
#include <ctime>
#include <stdio.h>

enum {
    XQHTOOLS_CSV_TO_HEX = 0,
    XQHTOOLS_CSV_TO_QHS,
    XQHTOOLS_QHS_TO_CSV,
    XQHTOOLS_QHS_MERGE,
};

// получаем команду из командной строки
int get_cmd(int &cmd, const int argc, char *argv[]);
// конвертируем csv в hex фалы
int csv_to_hex(const int argc, char *argv[]);
// конвертируем csv в qhs файлы
int csv_to_qhs(const int argc, char *argv[]);
// конвертировать хранилище котировок в csv файл
int qhs_to_csv(const int argc, char *argv[]);

int main(int argc, char *argv[]) {
    int cmd = 0;
    int err = get_cmd(cmd, argc, argv);
    if(err != 0) return err;

    if(cmd == XQHTOOLS_CSV_TO_HEX) {
        int err = csv_to_hex(argc, argv);
        if(err != 0) return err;
    } else
    if(cmd == XQHTOOLS_CSV_TO_QHS) {
        int err = csv_to_qhs(argc, argv);
        if(err != 0) return err;
    } else
    if(cmd == XQHTOOLS_QHS_TO_CSV) {
        int err = qhs_to_csv(argc, argv);
        if(err != 0) return err;
    }
    return 0;
}

int get_cmd(int &cmd, const int argc, char *argv[]) {
    // обрабатываем команду
    bool is_merge = false;
    bool is_convert_csv = false;
    bool is_convert_storage = false;
    bool is_hex = false;
    bool is_csv = false;
    bool is_storage = false;
    for(int i = 1; i < argc; ++i) {
        std::string value = std::string(argv[i]);
        if(value == "merge") is_merge = true;
        else
        if(value == "convert_csv") is_convert_csv = true;
        else
        if(value == "convert_storage") is_convert_storage = true;
        else
        if(value == "path_hex") is_hex = true;
        else
        if(value == "path_csv") is_csv = true;
        else
        if(value == "path_storage") is_storage = true;
    }
    if((is_merge && (is_convert_csv || is_convert_storage)) || (is_convert_csv && is_convert_storage)) {
        std::cout << "error! you have specified two conversion options" << std::endl;
        return -1;
    } else
    if(is_merge && is_hex) {
        std::cout << "error! hex file merging is not supported" << std::endl;
        return -1;
    } else
    if(is_merge && is_storage && !is_hex && !is_csv) {
        cmd = XQHTOOLS_QHS_MERGE;
    } else
    if(is_convert_csv && is_storage && is_csv && !is_hex) {
        cmd = XQHTOOLS_CSV_TO_QHS;
    } else
    if(is_convert_csv && !is_storage && is_csv && is_hex) {
        cmd = XQHTOOLS_CSV_TO_HEX;
    } else
    if(is_convert_storage && is_storage && is_csv && !is_hex) {
        cmd = XQHTOOLS_QHS_TO_CSV;
    } else {
        std::cout << "error! invalid parameters" << std::endl;
        return -1;
    }
    return 0;
}

int csv_to_hex(const int argc, char *argv[]) {
    int time_zone = xquotes_history::DO_NOT_CHANGE_TIME_ZONE;
    int type_price = xquotes_history::PRICE_OHLC;
    bool is_read_header = false;
    bool is_alpari = false;
    std::string path_hex = "";
    std::string path_csv = "";
    for(int i = 1; i < argc; ++i) {
        std::string value = std::string(argv[i]);
        if(value == "-cetgmt" || value == "-finam") time_zone = xquotes_history::CET_TO_GMT;
        else
        if(value == "-eetgmt") time_zone = xquotes_history::EET_TO_GMT;
        else
        if(value == "-alpari") is_alpari = true;
        else
        if(value == "-cetgmt") time_zone = xquotes_history::GMT_TO_CET;
        else
        if(value == "-eetgmt") time_zone = xquotes_history::GMT_TO_EET;
        else
        if(value == "-h") is_read_header = true;
        else
        if((value == "path_hex") && i < argc) {
            path_hex = std::string(argv[i + 1]);
        } else
        if((value == "path_csv") && i < argc) {
            path_csv = std::string(argv[i + 1]);
        } else
        if("-oc") {
            type_price = xquotes_history::PRICE_CLOSE;
        } else
        if(value == "-oo") {
            type_price = xquotes_history::PRICE_OPEN;
        } else
        if(value == "-ohlc") {
            type_price = xquotes_history::PRICE_OHLC;
        } else
        if(value == "-ohlcv") {
            type_price = xquotes_history::PRICE_OHLCV;
        }
    }

    if(path_hex == "" || path_csv == "" || path_hex == path_csv || bf::get_file_extension(path_hex) != "") {
        std::cout << "error! no path or directory specified" << std::endl;
        return -1;
    }
    if(is_alpari) {
        time_zone = xquotes_history::DO_NOT_CHANGE_TIME_ZONE;
    }

    const std::string file_extension =
        type_price == xquotes_history::PRICE_OHLC ? ".xq4hex" :
        type_price == xquotes_history::PRICE_OHLCV ? ".xq5hex" : ".xqhex";
    int err_csv = xquotes_csv::read_file(
            path_csv,
            is_read_header,
            time_zone,
            [&](xquotes_csv::Candle candle, bool is_end) {
        static int last_day = -1;
        static std::array<double, xtime::MINUTES_IN_DAY> price;
        static std::vector<xquotes_common::Candle> candles;
        static xtime::timestamp_t file_timestamp = 0;

        /* Торговые серверы ДЦ Альпари до 1 мая 2011 работали по СЕТ (центрально-европейское время),
         * после чего перешли на ЕЕТ (восточно- европейское время).
         */
        if(is_alpari) {
            const xtime::timestamp_t timestamp_alpari = xtime::convert_gmt_to_cet(xtime::get_timestamp(1,5,2011,23,59,59));
            xtime::timestamp_t timestamp = candle.timestamp;
            if(timestamp > timestamp_alpari) timestamp = xtime::convert_eet_to_gmt(timestamp);
            else timestamp = xtime::convert_cet_to_gmt(timestamp);
            candle.timestamp = timestamp;
        }

        int minute_day = xtime::get_minute_day(candle.timestamp);
        if(last_day == -1) { // момент инициализации
            last_day = xtime::get_day(candle.timestamp);
            file_timestamp = xtime::get_start_day(candle.timestamp);
            std::fill(price.begin(), price.end(), 0);
            price[minute_day] = type_price == xquotes_common::PRICE_CLOSE ?
                candle.close :
                type_price == xquotes_common::PRICE_OPEN ? candle.open :
                type_price == xquotes_common::PRICE_LOW ? candle.low :
                type_price == xquotes_common::PRICE_HIGH ? candle.high :
                candle.close;
            candles.push_back(candle);
        } else {
            int real_day = xtime::get_day(candle.timestamp);
            if(real_day != last_day || is_end) {
                last_day = real_day;
                // записываем данные тут
                std::cout << "write file: " << xtime::get_str_date(file_timestamp) << std::endl;
                if(
                    type_price == xquotes_history::PRICE_CLOSE ||
                    type_price == xquotes_history::PRICE_LOW ||
                    type_price == xquotes_history::PRICE_HIGH ||
                    type_price == xquotes_history::PRICE_OPEN) {
                    xquotes_files::write_bin_file_u32_1x(
                        path_hex + "//" + xquotes_files::get_file_name_from_date(file_timestamp) + file_extension,
                        price);
                } else
                if(type_price == xquotes_history::PRICE_OHLC) {
                    xquotes_files::write_bin_file_u32_4x(
                        path_hex + "//" + xquotes_files::get_file_name_from_date(file_timestamp) + file_extension,
                        candles);
                } else
                if(type_price == xquotes_history::PRICE_OHLCV) {
                    xquotes_files::write_bin_file_u32_5x(
                        path_hex + "//" + xquotes_files::get_file_name_from_date(file_timestamp) + file_extension,
                        candles);
                }
                //...
                std::fill(price.begin(), price.end(), 0);
                candles.clear();
                file_timestamp = xtime::get_start_day(candle.timestamp);
            }
            price[minute_day] = candle.close;
            candles.push_back(candle);
        }
    });
    if(err_csv != xquotes_common::OK) {
        std::cout << "error! error! csv file, code: " << err_csv << std::endl;
        return -1;
    }
    return 0;
}

int csv_to_qhs(const int argc, char *argv[]) {
    int time_zone = xquotes_history::DO_NOT_CHANGE_TIME_ZONE;
    int type_price = xquotes_history::PRICE_OHLC;
    int currency_pair = 0;
    bool is_read_header = false;
    bool is_alpari = false;
    bool is_compression = false;
    std::string path_storage = "";
    std::string path_csv = "";
    for(int i = 1; i < argc; ++i) {
        std::string value = std::string(argv[i]);
        if(value == "-cetgmt" || value == "-finam") time_zone = xquotes_history::CET_TO_GMT;
        else
        if(value == "-eetgmt") time_zone = xquotes_history::EET_TO_GMT;
        else
        if(value == "-alpari") is_alpari = true;
        else
        if(value == "-cetgmt") time_zone = xquotes_history::GMT_TO_CET;
        else
        if(value == "-eetgmt") time_zone = xquotes_history::GMT_TO_EET;
        else
        if(value == "-h") is_read_header = true;
        else
        if((value == "path_storage") && i < argc) {
            path_storage = std::string(argv[i + 1]);
        } else
        if((value == "path_csv") && i < argc) {
            path_csv = std::string(argv[i + 1]);
        } else
        if(value == "-oc") {
            type_price = xquotes_history::PRICE_CLOSE;
        } else
        if(value == "-oo") {
            type_price = xquotes_history::PRICE_OPEN;
        } else
        if(value == "-ohlc") {
            type_price = xquotes_history::PRICE_OHLC;
        } else
        if(value == "-ohlcv") {
            type_price = xquotes_history::PRICE_OHLCV;
        } else
        if(value == "-c") {
            is_compression = true;
        } else
        if(value == "-audcad") {
            currency_pair = xquotes_history::USE_DICTIONARY_AUDCAD;
        } else
        if(value == "-audchf") {
            currency_pair = xquotes_history::USE_DICTIONARY_AUDCHF;
        } else
        if(value == "-audjpy") {
            currency_pair = xquotes_history::USE_DICTIONARY_AUDJPY;
        } else
        if(value == "-audnzd") {
            currency_pair = xquotes_history::USE_DICTIONARY_AUDNZD;
        } else
        if(value == "-audusd") {
            currency_pair = xquotes_history::USE_DICTIONARY_AUDUSD;
        } else
        if(value == "-cadchf") {
            currency_pair = xquotes_history::USE_DICTIONARY_CADCHF;
        } else
        if(value == "-cadjpy") {
            currency_pair = xquotes_history::USE_DICTIONARY_CADJPY;
        } else
        if(value == "-chfjpy") {
            currency_pair = xquotes_history::USE_DICTIONARY_CHFJPY;
        } else
        if(value == "-euraud") {
            currency_pair = xquotes_history::USE_DICTIONARY_EURAUD;
        } else
        if(value == "-eurcad") {
            currency_pair = xquotes_history::USE_DICTIONARY_EURCAD;
        } else
        if(value == "-eurchf") {
            currency_pair = xquotes_history::USE_DICTIONARY_EURCHF;
        } else
        if(value == "-eurgbp") {
            currency_pair = xquotes_history::USE_DICTIONARY_EURGBP;
        } else
        if(value == "-eurjpy") {
            currency_pair = xquotes_history::USE_DICTIONARY_EURJPY;
        } else
        if(value == "-eurnok") {
            currency_pair = xquotes_history::USE_DICTIONARY_EURNOK;
        } else
        if(value == "-eurnzd") {
            currency_pair = xquotes_history::USE_DICTIONARY_EURNZD;
        } else
        if(value == "-eurusd") {
            currency_pair = xquotes_history::USE_DICTIONARY_EURUSD;
        } else
        if(value == "-gbpaud") {
            currency_pair = xquotes_history::USE_DICTIONARY_GBPAUD;
        } else
        if(value == "-gbpcad") {
            currency_pair = xquotes_history::USE_DICTIONARY_GBPCAD;
        } else
        if(value == "-gbpchf") {
            currency_pair = xquotes_history::USE_DICTIONARY_GBPCHF;
        } else
        if(value == "-gbpjpy") {
            currency_pair = xquotes_history::USE_DICTIONARY_GBPJPY;
        } else
        if(value == "-gbpnok") {
            currency_pair = xquotes_history::USE_DICTIONARY_GBPNOK;
        } else
        if(value == "-gbpnzd") {
            currency_pair = xquotes_history::USE_DICTIONARY_GBPNZD;
        } else
        if(value == "-gbpusd") {
            currency_pair = xquotes_history::USE_DICTIONARY_GBPUSD;
        } else
        if(value == "-nzdcad") {
            currency_pair = xquotes_history::USE_DICTIONARY_NZDCAD;
        } else
        if(value == "-nzdjpy") {
            currency_pair = xquotes_history::USE_DICTIONARY_NZDJPY;
        } else
        if(value == "-usdcad") {
            currency_pair = xquotes_history::USE_DICTIONARY_USDCAD;
        } else
        if(value == "-usdchf") {
            currency_pair = xquotes_history::USE_DICTIONARY_USDCHF;
        } else
        if(value == "-usdjpy") {
            currency_pair = xquotes_history::USE_DICTIONARY_USDJPY;
        } else
        if(value == "-usdnok") {
            currency_pair = xquotes_history::USE_DICTIONARY_USDNOK;
        } else
        if(value == "-usdpln") {
            currency_pair = xquotes_history::USE_DICTIONARY_USDPLN;
        }
    }
    if(path_storage == "" || path_csv == "" || path_storage == path_csv) {
        std::cout << "error! no path or directory specified" << std::endl;
        return -1;
    }
    if(is_alpari) {
        time_zone = xquotes_history::DO_NOT_CHANGE_TIME_ZONE;
    }

    std::string file_extension =
        type_price == xquotes_history::PRICE_OHLC ? ".qhs4" :
        type_price == xquotes_history::PRICE_OHLCV ? ".qhs5" : ".qhs";
    path_storage = bf::set_file_extension(path_storage, file_extension);

    int option = is_compression ? xquotes_history::USE_COMPRESSION : xquotes_history::DO_NOT_USE_COMPRESSION;
    xquotes_history::QuotesHistory<> iQuotesHistory(path_storage, xquotes_history::get_price_type_with_specific(type_price,currency_pair), option);

    int err_csv = xquotes_csv::read_file(
            path_csv,
            is_read_header,
            time_zone,
            [&](xquotes_csv::Candle candle, bool is_end) {
        static int last_day = -1;
        static std::vector<xquotes_common::Candle> candles;
        static xtime::timestamp_t file_timestamp = 0;

        /* Торговые серверы ДЦ Альпари до 1 мая 2011 работали по СЕТ (центрально-европейское время),
         * после чего перешли на ЕЕТ (восточно- европейское время).
         */
        if(is_alpari) {
            const xtime::timestamp_t timestamp_alpari = xtime::convert_gmt_to_cet(xtime::get_timestamp(1,5,2011,23,59,59));
            xtime::timestamp_t timestamp = candle.timestamp;
            if(timestamp > timestamp_alpari) timestamp = xtime::convert_eet_to_gmt(timestamp);
            else timestamp = xtime::convert_cet_to_gmt(timestamp);
            candle.timestamp = timestamp;
        }

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
                std::array<xquotes_common::Candle, xquotes_common::MINUTES_IN_DAY> new_candles;
                for(size_t i = 0; i < candles.size(); ++i) {
                    new_candles[xtime::get_minute_day(candles[i].timestamp)] = candles[i];
                }
                iQuotesHistory.write_candles(new_candles, file_timestamp);
                //...
                candles.clear();
                file_timestamp = xtime::get_start_day(candle.timestamp);
            }
            candles.push_back(candle);
        }
    });
    if(err_csv != xquotes_common::OK) {
        std::cout << "error! error! csv file, code: " << err_csv << std::endl;
        return -1;
    }
    return 0;
}

int qhs_to_csv(const int argc, char *argv[]) {
    int time_zone = xquotes_history::DO_NOT_CHANGE_TIME_ZONE;
    int type_csv = xquotes_csv::MT4;
    int type_correction_candle = xquotes_csv::SKIPPING_BAD_CANDLES;
    bool is_write_header = false;
    std::string path_storage = "";
    std::string path_csv = "";
    std::string header = "";
    for(int i = 1; i < argc; ++i) {
        std::string value = std::string(argv[i]);
        if(value == "-cetgmt" || value == "-finam") time_zone = xquotes_history::CET_TO_GMT;
        else
        if(value == "-eetgmt") time_zone = xquotes_history::EET_TO_GMT;
        else
        if(value == "-cetgmt") time_zone = xquotes_history::GMT_TO_CET;
        else
        if(value == "-eetgmt") time_zone = xquotes_history::GMT_TO_EET;
        else
        if((value == "header") && i < argc) {
            header = std::string(argv[i + 1]);
            is_write_header = true;
        }
        else
        if((value == "path_storage") && i < argc) {
            path_storage = std::string(argv[i + 1]);
        } else
        if((value == "path_csv") && i < argc) {
            path_csv = std::string(argv[i + 1]);
        } else
        if(value == "-m4") {
            type_csv = xquotes_csv::MT4;
        } else
        if(value == "-m5") {
            type_csv = xquotes_csv::MT5;
        } else
        if(value == "-ducascopy") {
            type_csv = xquotes_csv::DUKASCOPY;
        } else
        if(value == "-sbc") {
            type_correction_candle = xquotes_csv::SKIPPING_BAD_CANDLES;
        }
        if(value == "-fbc") {
            type_correction_candle = xquotes_csv::FILLING_BAD_CANDLES;
        }
        if(value == "-wbc") {
            type_correction_candle = xquotes_csv::WRITE_BAD_CANDLES;
        }
    }
    if(path_storage == "" || path_csv == "" || path_storage == path_csv) {
        std::cout << "error! no path or directory specified" << std::endl;
        return -1;
    }

    xquotes_history::QuotesHistory<> iQuotesHistory(path_storage, xquotes_history::PRICE_OHLC, xquotes_history::USE_COMPRESSION);
    xtime::timestamp_t min_timestamp = 0, max_timestamp = 0;
    int err = iQuotesHistory.get_min_max_day_timestamp(min_timestamp, max_timestamp);
    if(err != xquotes_history::OK) {
        std::cout << "error! error storage quotes, code: " << err << std::endl;
        return -1;
    }
    if(min_timestamp == 0 || max_timestamp == 0) {
        std::cout << "error! error storage quotes, no data available" << std::endl;
        return -1;
    }
    int decimal_places = 0;
    err = iQuotesHistory.get_decimal_places(decimal_places);
    if(err != xquotes_history::OK) {
        std::cout << "error! error storage quotes, code: " << err << std::endl;
        return -1;
    }
    if(decimal_places == 0) {
        std::cout << "error! error storage quotes, decimal places: " << decimal_places << std::endl;
        return -1;
    }

    std::cout << "write: " << path_csv << std::endl;
    if(is_write_header) std::cout << "write header: " << header << std::endl;
    int err_csv = xquotes_csv::write_file(
            path_csv,
            header,
            is_write_header,
            min_timestamp,
            max_timestamp + xtime::SECONDS_IN_DAY - 1,
            type_csv,
            type_correction_candle,
            time_zone,
            decimal_places,
            [&](xquotes_history::Candle &candle, const xtime::timestamp_t timestamp)->bool {
        int err_candle = iQuotesHistory.get_candle(candle, timestamp);
        if(timestamp % xtime::SECONDS_IN_DAY == 0) {
            std::cout << "write: " << path_csv << " " << xtime::get_str_date(timestamp) << std::endl;
        }
        if(err_candle != xquotes_history::OK && type_correction_candle == xquotes_csv::SKIPPING_BAD_CANDLES) return false;
        return true;
    });
    if(err_csv != xquotes_common::OK) {
        std::cout << "error! error! csv file, code: " << err_csv << std::endl;
        return -1;
    }
    return 0;
}
