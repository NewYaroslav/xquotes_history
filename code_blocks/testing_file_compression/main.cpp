#include "xquotes_quotation_files.hpp"
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

    return 0;
}

