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
    /* Проверяем работу хранилищая котировок для нескольких символов
     */
    std::vector<std::string> paths = {"../../storage/AUDCHF.qhs4", "../../storage/AUDUSD.qhs4", "../../storage/EURCHF.qhs4", "../../storage/EURGBP.qhs4", "../../storage/EURJPY.qhs4"};
    /* инициализируем класс для хранения исторических данных котировок с неправильными параметрами
     * если класс работает верно, он загрузит параметры из файла, и не будет использовать указанные в
     * конструкторе параметры
     */
    xquotes_history::MultipleQuotesHistory<> iMultipleQuotesHistory(
            paths,
            xquotes_history::PRICE_CLOSE,
            xquotes_history::DO_NOT_USE_COMPRESSION);

    // получаем количество символов
    std::cout << "num symbols: " << iMultipleQuotesHistory.get_num_symbols() << std::endl;
    // получаем минимальную и максимальную дату начала дня котировок
    xtime::timestamp_t min_timestamp = 0, max_timestamp = 0;
    iMultipleQuotesHistory.get_min_max_day_timestamp(min_timestamp, max_timestamp);
    std::cout << "start day timestamp: " << xtime::get_str_date_time(min_timestamp) << " - " << xtime::get_str_date_time(max_timestamp) << std::endl;

    // читаем бар
    xquotes_history::Candle candle1;
    std::cout << "checking the reading of the same price" << std::endl;
    iMultipleQuotesHistory.get_candle(candle1, xtime::get_timestamp(1, 3, 2018, 12, 30, 0), 0);
    std::cout << "candle #1, open: " << candle1.open << " close: " << candle1.close << " date: " << xtime::get_str_date_time(candle1.timestamp) << std::endl;

    /* получаем список меток времени начала дня тех данных, которые есть в хранилище
     * (т.е. например выходные там отсутствуют, их в списке не будет)
     */
    std::vector<xtime::timestamp_t> list_timestamp;
    std::vector<bool> is_symbol(paths.size(), true);
    int err = iMultipleQuotesHistory.get_day_timestamp_list(list_timestamp, xtime::get_timestamp(1, 1, 2018), is_symbol, 10, true, false);
    std::cout << "get_start_day_timestamp_list err: " << err << std::endl;
    for(size_t i = 0; i < list_timestamp.size(); ++i) {
        std::cout << "date: " << xtime::get_str_date_time(list_timestamp[i]) << std::endl;
    }

    /* Проверяем лямбду функцию для одновременной торговли на нескольких символах
     */
    int err_trade = iMultipleQuotesHistory.trade(
        xtime::get_timestamp(1, 2, 2018),
        xtime::SECONDS_IN_MINUTE*60,
        2,
        is_symbol,
        [&](const xquotes_history::Candle &candle,
            const int day,
            const int symbol_indx,
            const int err) {
        // обращаемся из лямбда-функции к классу MultipleQuotesHistory, например к методу get_number_decimal_places
        int decimal_places = 0;
        iMultipleQuotesHistory.get_decimal_places(decimal_places, symbol_indx);
        // мы не торгуем здесь, просто выведем информацию
        std::cout << "step: " << xtime::get_str_date_time(candle.timestamp) << " day: " << day << " symbol: " << paths[symbol_indx] << " precision " << decimal_places << " err: " << err << std::endl;
    });
    std::cout << "trade err: " << err_trade << std::endl;

    /* Проверяем лямбду функцию для одновременной торговли на нескольких символах в несколько потоков
     */
    std::mutex thread_mutex;
    int err_trade_multiple_threads = iMultipleQuotesHistory.trade_multiple_threads(
        xtime::get_timestamp(1, 2, 2018),
        xtime::SECONDS_IN_MINUTE*60,
        2,
        is_symbol,
        [&](const xquotes_history::Candle &candle,
            const int day,
            const int symbol_indx,
            const int thread_indx,
            const int err) {
        // обращаемся из лямбда-функции к классу MultipleQuotesHistory, например к методу get_number_decimal_places
        int decimal_places = 0;
        iMultipleQuotesHistory.get_decimal_places(decimal_places, symbol_indx);
        // мы не торгуем здесь, просто выведем информацию
        thread_mutex.lock();
        std::cout
            << "date: " << xtime::get_str_date_time(candle.timestamp)
            << " day: " << day
            << " symbol: "
            << paths[symbol_indx]
            << " prec: " << decimal_places
            << " thread: " << thread_indx
            << " err: " << err << std::endl;
        thread_mutex.unlock();
    });
    std::cout << "trade multiple threads err: " << err_trade_multiple_threads << std::endl;

    for(size_t i = 0; i < paths.size(); ++i) {
        int decimal_places = 0;
        int err = iMultipleQuotesHistory.get_decimal_places(decimal_places,i);
        std::cout << "symbol " << paths[i] << " decimal places: " << decimal_places << " err " << err << std::endl;
    }

    std::cout << "coarsening price: " << xquotes_history::coarsening_price(1.07208,10,true) << std::endl;
    std::cout << "coarsening price: " << xquotes_history::coarsening_price(1.07208,100000,true) << std::endl;
    std::cout << "compare price: " << xquotes_history::compare_price(1.07208,xquotes_history::coarsening_price(1.07208,10,true),10) << std::endl;
    std::cout << "compare price: " << xquotes_history::compare_price(1.07208,xquotes_history::coarsening_price(1.07208,100,true),100) << std::endl;
    std::cout << "compare price: " << xquotes_history::compare_price(1.07208,xquotes_history::coarsening_price(1.07208,1000,true),1000) << std::endl;
    std::cout << "compare price: " << xquotes_history::compare_price(1.07208,xquotes_history::coarsening_price(1.07208,100000,true),100000) << std::endl;

    std::cout << "get fractional: " << xquotes_history::get_fractional(1.07208,100000,true) << std::endl;
    std::cout << "get fractional: " << xquotes_history::get_fractional(1.07208,5,false) << std::endl;

    std::cout << ": " << (xquotes_history::get_fractional(1.07208,5,false) % 10) << std::endl;
    system("pause");
    return 0;
}


