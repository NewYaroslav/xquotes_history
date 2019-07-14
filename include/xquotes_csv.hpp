/*
* xquotes_history - C++ header-only library for working with quotes
*
* Copyright (c) 2018 Elektro Yar. Email: git.electroyar@gmail.com
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/
#ifndef XQUOTES_CSV_HPP_INCLUDED
#define XQUOTES_CSV_HPP_INCLUDED

#include "xquotes_common.hpp"
#include "banana_filesystem.hpp"
#include "xtime.hpp"
#include <functional>
#include <iostream>

namespace xquotes_csv {
    using namespace xquotes_common;

    class Candle {
    public:
        double open = 0;
        double high = 0;
        double low = 0;
        double close = 0;
        unsigned long long timestamp = 0;
        Candle() {};

        Candle(double open, double high, double low, double close, unsigned long long timestamp) {
            Candle::open = open;
            Candle::high = high;
            Candle::low = low;
            Candle::close = close;
            Candle::timestamp = timestamp;
        }
    };

    int read_file(
            std::string file_name,
            bool is_read_header,
            bool is_cet_to_gmt,
            std::function<void (Candle candle)> f
            ) {
        std::ifstream file(file_name);
        if (!file.is_open()) {
            return FILE_CANNOT_OPENED;
        }
        std::string buffer;
        unsigned long long last_unix = 0;
        // получаем заголовок файла
        if(is_read_header) std::getline(file, buffer);
        while(!file.eof()) {
            std::getline(file, buffer);
            std::size_t offset = 0;
            std::size_t found = buffer.find_first_of(";,", offset);
            if(found == std::string::npos) break;

            std::string str_data = buffer.substr(offset, found - offset);

            offset = found + 1;
            found = buffer.find_first_of(";,", offset);
            std::string str_data2 = buffer.substr(offset, found - offset);

            xtime::DateTime iTime(
                atoi((str_data.substr(8, 2)).c_str()),
                atoi((str_data.substr(5, 2)).c_str()),
                atoi((str_data.substr(0, 4)).c_str()),
                atoi((str_data2.substr(0, 2)).c_str()),
                atoi((str_data2.substr(3, 2)).c_str()),
                0);

            // price open
            offset = found + 1;
            found = buffer.find_first_of(";,", offset);
            if(found == std::string::npos) break;
            double open = atof((buffer.substr(offset, found - offset)).c_str());
            // price high
            offset = found + 1;
            found = buffer.find_first_of(";,", offset);
            if(found == std::string::npos) break;
            double high = atof((buffer.substr(offset, found - offset)).c_str());
            // price low
            offset = found + 1;
            found = buffer.find_first_of(";,", offset);
            if(found == std::string::npos) break;
            double low = atof((buffer.substr(offset, found - offset)).c_str());
            // price close
            offset = found + 1;
            found = buffer.find_first_of(";,", offset);
            if(found == std::string::npos) break;
            double close = atof((buffer.substr(offset, found - offset)).c_str());

            unsigned long long unix = iTime.get_timestamp();
            if(is_cet_to_gmt) unix = xtime::convert_cet_to_gmt(unix);
            f(Candle(open, high, low, close, unix));

            /*
            if(last_unix == unix && candles.size() > 0) {
                candles[candles.size() - 1] = Candle(open, high, low, close, unix);
                continue;
            } else {
                candles.push_back(Candle(open, high, low, close, unix));
                last_unix = unix;
            }
            */
        }
        file.close();
        return OK;
    }
}
#endif // XQUOTES_CSV_HPP_INCLUDED
