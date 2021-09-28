#include <iostream>
#include "xquotes_history.hpp"
#include <vector>
#include <array>
//#include <iostream>
#include <random>
#include <ctime>
#include <stdio.h>
#include <ctime>

int main(int argc, char *argv[]) {
    std::cout << "start!" << std::endl;
    /* Проверяем работу хранилищая котировок
     * Прочитаем данные Финама и сверим их с данными в Метатрейдере
     */
    std::string path = "../../storage/EURGBP.qhs4"; // путь к файлу

    /* инициализируем класс для хранения исторических данных котировок с неправильными параметрами
     * если класс работает верно, он загрузит параметры из файла, и не будет использовать указанные в
     * конструкторе параметры
     */
    xquotes_history::QuotesHistory<> iQuotesHistory(
            path,
            xquotes_history::PRICE_CLOSE,
            xquotes_history::DO_NOT_USE_COMPRESSION);

    // вернет число 18, что означает - бит компрессии = 0x10 + PRICE_OHLC = 0x02
    std::cout << "note " << iQuotesHistory.get_file_note() << std::endl;

    xquotes_history::Candle candle1, candle2, candle3;

    std::cout << "checking the reading of the same price" << std::endl;
    // делаем запрос цены несколько раз за конкретную дату, цена open должна быть 1.21887, цена close 1.21894 для даты 01.03.2018 12:30 по CET
    iQuotesHistory.get_candle(candle1, ztime::convert_cet_to_gmt(ztime::get_timestamp(1, 3, 2018, 12, 30, 0)));
    std::cout << "candle #1, open: " << candle1.open << " close: " << candle1.close << " date: " << ztime::get_str_date_time(candle1.timestamp) << std::endl;
    iQuotesHistory.get_candle(candle2, ztime::convert_cet_to_gmt(ztime::get_timestamp(1, 3, 2018, 12, 30, 0)));
    std::cout << "candle #2, open: " << candle2.open << " close: " << candle2.close << " date: " << ztime::get_str_date_time(candle2.timestamp) << std::endl;
    iQuotesHistory.get_candle(candle3, ztime::convert_cet_to_gmt(ztime::get_timestamp(1, 3, 2018, 12, 30, 0)));
    std::cout << "candle #2, open: " << candle3.open << " close: " << candle3.close << " date: " << ztime::get_str_date_time(candle3.timestamp) << std::endl;

    std::cout << "sequential read" << std::endl;
    // делаем последовательный запрос цены по дням
    for(int i = 1; i < 10; ++i) {
        xquotes_history::Candle candle;
        iQuotesHistory.get_candle(candle, ztime::convert_cet_to_gmt(ztime::get_timestamp(i, 3, 2018, 12, 30, 0)));
        std::cout << "candle #" << i << ", open: " << candle.open << " close: " << candle.close << " date: " << ztime::get_str_date_time(candle.timestamp) << std::endl;
    }
    // делаем последовательный запрос цены по минуте
    for(int i = 0; i < 10; ++i) {
        xquotes_history::Candle candle;
        iQuotesHistory.get_candle(candle, ztime::convert_cet_to_gmt(ztime::get_timestamp(1, 3, 2018, 12, 0 + i, 0)));
        std::cout << "candle #" << i << ", open: " << candle.open << " close: " << candle.close << " date: " << ztime::get_str_date_time(candle.timestamp) << std::endl;
    }
    // делаем запрос цены из далекого прошлого
    std::cout << "check the time limit" << std::endl;
    iQuotesHistory.get_candle(candle1, ztime::convert_cet_to_gmt(ztime::get_timestamp(1, 3, 1980, 12, 30, 0)));
    std::cout << "candle past, open: " << candle1.open << " close: " << candle1.close << " date: " << ztime::get_str_date_time(candle1.timestamp) << std::endl;
    iQuotesHistory.get_candle(candle2, ztime::convert_cet_to_gmt(ztime::get_timestamp(1, 3, 4000, 12, 30, 0)));
    std::cout << "candle future, open: " << candle2.open << " close: " << candle2.close << " date: " << ztime::get_str_date_time(candle2.timestamp) << std::endl;


    std::cout << "start day timestamp list" << std::endl;
    std::vector<ztime::timestamp_t> list_timestamp;
    int err_list = iQuotesHistory.get_day_timestamp_list(
        list_timestamp,
        ztime::get_timestamp(1, 3, 2018),
        10);
    std::cout << "err: " << err_list << std::endl;

    std::cout << "start timestamp: " << ztime::get_str_date_time(ztime::get_timestamp(1, 3, 2018)) << std::endl;
    for(size_t i = 0; i < list_timestamp.size(); ++i) {
        std::cout << "day #" << i << " - " << ztime::get_str_date_time(list_timestamp[i]) << " is day off: "<< ztime::is_day_off(list_timestamp[i]) << std::endl;
    }

    /* Проверим, как работает алгоритм получения количесвта десятичных знаков после запятой
     *
     */
    std::string path2 = "../../storage/EURJPY.qhs4";
    xquotes_history::QuotesHistory<> iQuotesHistory2(
            path2,
            xquotes_history::PRICE_CLOSE,
            xquotes_history::DO_NOT_USE_COMPRESSION);
    int decimal_places = 0;
    iQuotesHistory2.get_decimal_places(decimal_places);
    std::cout << "decimal places: " << decimal_places << std::endl;

    system("pause");
    return 0;
}


