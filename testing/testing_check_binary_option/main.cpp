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
    std::string path = "AUDUSD.qhs4"; // путь к файлу

    /* инициализируем класс для хранения исторических данных котировок с неправильными параметрами
     * если класс работает верно, он загрузит параметры из файла, и не будет использовать указанные в
     * конструкторе параметры
     */
    xquotes_history::QuotesHistory<> iQuotesHistory(
            path,
            xquotes_history::PRICE_OHLC,
            xquotes_history::USE_COMPRESSION);

    xquotes_history::Candle start_candle;
    xquotes_history::Candle stop_candle;
    xquotes_history::Candle test_candle;
    iQuotesHistory.get_candle(start_candle, xtime::get_timestamp(1,3,2018,7,0,0));
    iQuotesHistory.get_candle(stop_candle, xtime::get_timestamp(1,3,2018,7,3,0));
    iQuotesHistory.get_candle(test_candle, xtime::get_timestamp(1,3,2018,7,0,0) + 3*xtime::SECONDS_IN_MINUTE);
    std::cout << start_candle.close << " " << stop_candle.close << " " << test_candle.close << std::endl;
    int state = xquotes_history::NEUTRAL;
    int err_bo = iQuotesHistory.check_binary_option(state, xquotes_history::SELL, 3*xtime::SECONDS_IN_MINUTE, xtime::get_timestamp(1,3,2018,7,0,0));
    std::cout << "err_bo " << err_bo << " " << state << std::endl;
    system("pause");
    return 0;
}


