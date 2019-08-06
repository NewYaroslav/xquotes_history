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

    bool get_line_param(
            const std::string &str,
            unsigned long long &timestamp,
            double &open,
            double &high,
            double &low,
            double &close,
            double &volume) {
            std::size_t offset = 0;
            std::size_t found = str.find_first_of(";,", offset);
            if(found == std::string::npos) return false;
            std::string str_date = str.substr(offset, found - offset);
            if(str_date.size() == 10) {
                // для формата на примере 2018.05.22,21:31
                offset = found + 1;
                found = str.find_first_of(";,", offset);
                std::string str_time = str.substr(offset, found - offset);

                xtime::DateTime iTime(
                    atoi((str_date.substr(8, 2)).c_str()),
                    atoi((str_date.substr(5, 2)).c_str()),
                    atoi((str_date.substr(0, 4)).c_str()),
                    atoi((str_time.substr(0, 2)).c_str()),
                    atoi((str_time.substr(3, 2)).c_str()),
                    0);
                timestamp = iTime.get_timestamp();
            } else
            if(str_date.size() == 23) {
                // для формата на примере 03.10.2018 12:43:00.000

                xtime::DateTime iTime(
                    atoi((str_date.substr(0, 2)).c_str()),
                    atoi((str_date.substr(3, 2)).c_str()),
                    atoi((str_date.substr(6, 4)).c_str()),
                    atoi((str_date.substr(11, 2)).c_str()),
                    atoi((str_date.substr(14, 2)).c_str()),
                    atoi((str_date.substr(17, 2)).c_str()));
                timestamp = iTime.get_timestamp();
            } else {
                std::cout << "error dae time size: " << str_date.size() << std::endl;
                return false;
            }
            // price open
            offset = found + 1;
            found = str.find_first_of(";,", offset);
            if(found == std::string::npos) return false;
            open = atof((str.substr(offset, found - offset)).c_str());
            // price high
            offset = found + 1;
            found = str.find_first_of(";,", offset);
            if(found == std::string::npos) return false;
            high = atof((str.substr(offset, found - offset)).c_str());
            // price low
            offset = found + 1;
            found = str.find_first_of(";,", offset);
            if(found == std::string::npos) return false;
            low = atof((str.substr(offset, found - offset)).c_str());
            // price close
            offset = found + 1;
            found = str.find_first_of(";,", offset);
            if(found == std::string::npos) return false;
            close = atof((str.substr(offset, found - offset)).c_str());
            volume = atof((str.substr(found + 1)).c_str());

            //std::cout << open << " " << high << " " << low << " " << close << " " << volume << std::endl;
            return true;
        }

    int read_file(
            std::string file_name,
            bool is_read_header,
            bool is_cet_to_gmt,
            std::function<void (Candle candle, bool is_end)> f
            ) {
        std::ifstream file(file_name);
        if (!file.is_open()) {
            return FILE_CANNOT_OPENED;
        }
        std::string buffer;

        // получаем заголовок файла
        if(is_read_header) std::getline(file, buffer);

        while(!file.eof()) {
            std::getline(file, buffer);
            unsigned long long timestamp;
            double open, high, low, close, volume;
            if(!get_line_param(buffer, timestamp, open, high, low, close, volume)) {
                f(Candle(open, high, low, close, volume, timestamp), true);
                break;
            }
            if(is_cet_to_gmt) timestamp = xtime::convert_cet_to_gmt(timestamp);
            f(Candle(open, high, low, close, volume, timestamp), false);
        }
        file.close();
        return OK;
    }
}
#endif // XQUOTES_CSV_HPP_INCLUDED
