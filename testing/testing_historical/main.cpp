#include <iostream>
#include "xquotes_history.hpp"
#include <vector>
#include <array>
//#include <iostream>
#include <random>
#include <ctime>
#include <stdio.h>
#include <ctime>

void get_rnd_data(std::array<xquotes_history::Candle, xquotes_history::MINUTES_IN_DAY>& candles) {
    std::mt19937 gen;
    gen.seed(time(0));
    std::uniform_int_distribution<> denominator(-99999, 99999);
    const double START_NUMERATOR = 60.0;
    candles[0].close = START_NUMERATOR + (double)denominator(gen)/100000.0;
    for(size_t i = 1; i < candles.size(); ++i) {
        candles[i].close = candles[i - 1].close + (double)denominator(gen)/100000.0;
    }
}

int main(int argc, char *argv[]) {
    std::cout << "start!" << std::endl;
    /* Проверяем работу хранилищая котировок, сначала запишем сжатые данные, потом прочитаем
     * Еще проверяем разницу между словарем, который есть в постоянной памяти, и словарем в ОЗУ
     */

    std::cout << "iQuotesHistory" << std::endl;
    xquotes_history::QuotesHistory<> iQuotesHistory(
        "test.dat", xquotes_history::PRICE_CLOSE,
        (const char*)xquotes_dictionary::dictionary_only_one_price,
        sizeof(xquotes_dictionary::dictionary_only_one_price));

    // добалвяем словарь в ОЗУ и создаем еще один файл
    char *new_dictionary_only_one_price = new char[sizeof(xquotes_dictionary::dictionary_only_one_price)];
    memcpy(new_dictionary_only_one_price, xquotes_dictionary::dictionary_only_one_price, sizeof(xquotes_dictionary::dictionary_only_one_price));

    std::cout << "iQuotesHistory2" << std::endl;
    xquotes_history::QuotesHistory<> iQuotesHistory2(
        "test2.dat", xquotes_history::PRICE_CLOSE,
        (const char*)new_dictionary_only_one_price,
        sizeof(xquotes_dictionary::dictionary_only_one_price));

    std::cout << "iQuotesHistory3" << std::endl;
    xquotes_history::QuotesHistory<> iQuotesHistory3(
        "test3.dat",
        xquotes_history::PRICE_CLOSE,
        //"..\\..\\dictionary\\only_one_price.dat");
        "");

    std::cout << "iQuotesHistory4" << std::endl;
    xquotes_history::QuotesHistory<> iQuotesHistory4(
        "test4.dat",
        xquotes_history::PRICE_CLOSE,
        xquotes_history::USE_COMPRESSION);

    std::cout << "samples" << std::endl;
    // время
    double aver_t1 = 0, aver_t2 = 0;
    const int NUM_SAMPLES = 2000;
    std::vector<std::array<xquotes_history::Candle, xquotes_history::MINUTES_IN_DAY>> array_candles;
    for(int i = 0; i < NUM_SAMPLES; ++i) {
        std::array<xquotes_history::Candle, xquotes_history::MINUTES_IN_DAY> candles;
        get_rnd_data(candles);
        array_candles.push_back(candles);
        unsigned long long start_time;
        unsigned long long delay_time;

        start_time = clock();
        iQuotesHistory.write_candles(candles, xtime::get_timestamp(i + 1,8,2019,0,0,0));
        delay_time = clock() - start_time;
        double t1 = (double)delay_time / (double)CLOCKS_PER_SEC;
        aver_t1 += t1;
        start_time = clock();
        iQuotesHistory2.write_candles(candles, xtime::get_timestamp(i + 1,8,2019,0,0,0));
        delay_time = clock() - start_time;
        double t2 = (double)delay_time / (double)CLOCKS_PER_SEC;
        aver_t2 += t2;

        iQuotesHistory3.write_candles(candles, xtime::get_timestamp(i + 1,8,2019,0,0,0));
        iQuotesHistory4.write_candles(candles, xtime::get_timestamp(i + 1,8,2019,0,0,0));
        std::cout << "write " << xtime::get_timestamp(i + 1,8,2019,0,0,0) << std::endl;
        system("pause");
    }
    aver_t1 /= (double)NUM_SAMPLES;
    aver_t2 /= (double)NUM_SAMPLES;
    std::cout << "aver t1 " << 1000000* aver_t1 << std::endl;
    std::cout << "aver t2 " << 1000000* aver_t2 << std::endl;

    std::cout << "check!" << std::endl;
    std::cout << "array_candles size: " << array_candles.size() << std::endl;
    int offset_minute_day = 145;
    std::cout << "check test close price: " << array_candles[0][offset_minute_day].close << std::endl;
    xtime::timestamp_t min_timestamp = 0, max_timestamp = 0;
    iQuotesHistory.get_min_max_start_day_timestamp(min_timestamp, max_timestamp);
    std::cout << "min max timestamp: " << min_timestamp << " - " << max_timestamp << std::endl;

    /* Проверяем доступ к данным
     * Если что то пойдет не так, увидим сообщение об ошибке
     * Иначе ничего не видим
     */
    for(int o = 0; o < 20; ++o)
    for(int i = 0; i < 20; ++i) {
        xquotes_history::Candle candle;
        offset_minute_day = 145 + o;
        //std::cout << "check " << xtime::get_timestamp(i + 1,8,2019,0,0,0) + offset_minute_day * xtime::SECONDS_IN_MINUTE << std::endl;
        int err = iQuotesHistory.get_candle(candle, xtime::get_timestamp(i + 1,8,2019,0,0,0) + offset_minute_day * xtime::SECONDS_IN_MINUTE);
        if(err != 0) {
            std::cout << "error! code: " << err << std::endl;
        } else {
            double temp = array_candles[i][offset_minute_day].close - candle.close;
            if(temp > 0.000001) {
                std::cout << "error! " << array_candles[i][offset_minute_day].close << "/" << candle.close << std::endl;
            } else {
                //std::cout << "ok!" << std::endl;
            }
        }
    }

    /* Проверяем торговлю через лямбду функцию
     *
     */
    iQuotesHistory.trade(
        xtime::get_timestamp(1,8,2019,0,0,0),
        xtime::get_timestamp(3,8,2019,0,0,0),
        xtime::SECONDS_IN_MINUTE,
        [&](const xquotes_history::QuotesHistory<> &hist,
            const xquotes_history::Candle &candle,
            const int err){
            if(xtime::is_beg_hour(candle.timestamp))
                std::cout << "close " << candle.close << " timestamp " << candle.timestamp << std::endl;
        });

    delete new_dictionary_only_one_price;
    system("pause");
    return 0;
}


