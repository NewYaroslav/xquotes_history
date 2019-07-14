#include "xquotes_quotation_files.hpp"
#include "xquotes_csv.hpp"
#include <vector>
#include <array>
#include <iostream>
#include <random>
#include <ctime>
#include <stdio.h>

int main() {
    std::vector<double> prices(xtime::MINUTES_IN_DAY);
    std::mt19937 gen;
    gen.seed(time(0));
    std::uniform_int_distribution<> denominator(-99999, 99999);
    const double START_NUMERATOR = 130.0;
    prices[0] = START_NUMERATOR + (double)denominator(gen)/100000.0;
    for(size_t i = 1; i < prices.size(); ++i) {
        prices[i] = prices[i - 1] + (double)denominator(gen)/100000.0;
    }

    for(size_t i = 0; i < prices.size(); ++i) {
        printf("%.5f\n", prices[i]);
    }

    xquotes_history::write_binary_file("text.hex", prices);
    std::array<double, 1440> read_prices;
    xquotes_history::read_binary_file("text.hex", read_prices);
    for(size_t i = 0; i < read_prices.size(); ++i) {
        int rvalue = (int)(prices[i] * 100000.0 + 0.5);
        int lvalue = (int)(read_prices[i] * 100000.0 + 0.5);
        if(lvalue != rvalue) {
            printf("error %.5f %.5f\n%d %d\n", prices[i], read_prices[i], rvalue, lvalue);

        }
    }

#if(0)
    std::vector<xquotes_csv::Candle> candles;
    std::cout << xquotes_csv::read_file("..\\..\\csv\\EURUSD1.csv", true, true, [&](xquotes_csv::Candle candle) {
         candles.push_back(candle);
    }) << std::endl;
    std::cout << "size: " << candles.size() << std::endl;
#endif

    int err_csv = xquotes_csv::read_file("..\\..\\csv\\EURUSD1.csv", true, true, [&](xquotes_csv::Candle candle) {
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
                std::cout << "new day " << xtime::get_str_unix_date_time(file_timestamp) << std::endl;

                //...
                std::fill(close.begin(), close.end(), 0);
                file_timestamp = candle.timestamp;
            }
            close[minute_day] = candle.close;
        }

    });

    return 0;
}

