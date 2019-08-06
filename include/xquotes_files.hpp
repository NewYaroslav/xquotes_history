/*
* xquotes_history - C++ header-only library for working with historical quotes data
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
#ifndef XQUOTES_FILES_HPP_INCLUDED
#define XQUOTES_FILES_HPP_INCLUDED

#include "xquotes_common.hpp"
#include "banana_filesystem.hpp"
#include "xtime.hpp"
#include <limits>
#include <algorithm>
#include <random>
#include <cstdio>

#include <iostream>

namespace xquotes_files {
    using namespace xquotes_common;
//------------------------------------------------------------------------------
// РАБОТА С ОДИНОЧНЫМИ ФАЙЛАМИ
//------------------------------------------------------------------------------
    /** \brief Получить имя файла из даты
     * Выбрана последовательность ГОД МЕСЯЦ ДЕНЬ чтобы файлы были
     * в алфавитном порядке
     * \param timestamp временная метка
     * \return имя файла
     */
    std::string get_file_name_from_date(unsigned long long timestamp) {
        xtime::DateTime iTime(timestamp);
        std::string file_name =
            std::to_string(iTime.year) + "_" +
            std::to_string(iTime.month) + "_" +
            std::to_string(iTime.day);
        return file_name;
    }

    inline void write_u32(std::ofstream &file, const double &value) {
        price_t temp = convert_to_uint(value);
        file.write(reinterpret_cast<char *>(&temp), sizeof(price_t));
    }

    inline void write_null_u32(std::ofstream &file) {
        price_t PRICE_NULL = 0;
        file.write(reinterpret_cast<char *>(&PRICE_NULL), sizeof(price_t));
    }

    inline double read_u32(std::ifstream &file) {
        price_t temp = 0;
        file.read(reinterpret_cast<char *>(&temp), sizeof(price_t));
        return convert_to_double(temp);
    }

    /** \brief Записать бинарный файл котировок
     * Данная функция заприсывает котировки только для одной цены, например для close
     * \param file_name имя файла
     * \param prices котировки
     * \return вернет 0 в случае успешного завершения
     */
    template <typename T>
    int write_bin_file_u32_1x(
            std::string file_name,
            T &prices) {
        if(prices.size() != MINUTES_IN_DAY) return INVALID_ARRAY_LENGH;
        std::ofstream file(file_name, std::ios_base::binary);
        for(int i = 0; i < MINUTES_IN_DAY; ++i) {
            write_u32(file, prices[i]);
        }
        file.close();
        return OK;
    }

    template <typename T>
    inline void write_candle_u32_4x(std::ofstream &file, const T &candle, const int &indx) {
        write_u32(file, candle.open);
        write_u32(file, candle.high);
        write_u32(file, candle.low);
        write_u32(file, candle.close);
    }

    inline void write_null_candle_u32_4x(std::ofstream &file) {
        for(int i = 0; i < 4; ++i) {
            write_null_u32(file);
        }
    }

    inline void write_null_candle_u32_5x(std::ofstream &file) {
        for(int i = 0; i < 4; ++i) {
            write_null_u32(file);
        }
    }

    template <typename T>
    inline void write_candle_u32_5x(std::ofstream &file, const T &candle, const int &indx) {
        write_u32(file, candle.open);
        write_u32(file, candle.high);
        write_u32(file, candle.low);
        write_u32(file, candle.close);
        write_u32(file, candle.volume);
    }

    /** \brief Записать бинарный файл котировок
     * Данная функция может принимать котировки с пропусками временых меток
     * \param file_name имя файла
     * \param prices котировки
     * \param times временные метки
     * \return вернет 0 в случае успешного завершения
     */
    int write_bin_file_u32_1x(std::string file_name,
            std::vector<double> &prices,
            std::vector<xtime::timestamp_type> &times) {
        if(prices.size() != times.size() || prices.size() == 0) return INVALID_ARRAY_LENGH;
        std::ofstream file(file_name, std::ios_base::binary);
        xtime::DateTime iTime(times[0]);
        iTime.set_beg_day();
        xtime::timestamp_type timestamp = iTime.get_timestamp();
        size_t times_indx = 0;
        for(size_t i = 0; i < MINUTES_IN_DAY; ++i) {
            if(times_indx >= times.size()) {
                write_null_u32(file);
            } else
            if(times[times_indx] == timestamp) {
                write_u32(file, prices[times_indx]);
                times_indx++;
            } else
            if(times[times_indx] > timestamp) {
                write_null_u32(file);
            }
            timestamp += xtime::SECONDS_IN_MINUTE;
        }
        file.close();
        return OK;
    }

    /** \brief Записать бинарный файл котировок
     * Данная функция может принимать котировки с пропусками временых меток
     * Данная функция записывает котировки для 4-х значений цен (цены свечи\бара)
     * \param file_name имя файла
     * \param candles котировки в виде массива свечей\баров
     * \param times временные метки
     * \return вернет 0 в случае успешного завершения
     */
    template <typename T>
    int write_bin_file_u32_4x(std::string file_name, T &candles) {
        if(candles.size() == 0) return INVALID_ARRAY_LENGH;
        std::ofstream file(file_name, std::ios_base::binary);
        xtime::DateTime iTime(candles[0].timestamp);
        iTime.set_beg_day();
        unsigned long long timestamp = iTime.get_timestamp();
        size_t indx = 0;
        for(size_t i = 0; i < MINUTES_IN_DAY; ++i) {
            if(indx >= candles.size()) {
                write_null_candle_u32_4x(file);
            } else
            if(candles[indx].timestamp == timestamp) {
                write_candle_u32_4x(file, candles[i], i);
                indx++;

            } else
            if(candles[indx].timestamp > timestamp) {
                write_null_candle_u32_4x(file);
            }
            timestamp += xtime::SECONDS_IN_MINUTE;
        } // for i
        file.close();
        return OK;
    }

    /** \brief Записать бинарный файл котировок
     * Данная функция может принимать котировки с пропусками временых меток
     * Данная функция записывает котировки для 4-х значений цен (цены свечи\бара) и объем
     * \param file_name имя файла
     * \param candles котировки в виде массива свечей\баров
     * \param times временные метки
     * \return вернет 0 в случае успешного завершения
     */
    template <typename T>
    int write_bin_file_u32_5x(std::string file_name, T &candles) {
        if(candles.size() == 0) return INVALID_ARRAY_LENGH;
        std::ofstream file(file_name, std::ios_base::binary);
        xtime::DateTime iTime(candles[0].timestamp);
        iTime.set_beg_day();
        unsigned long long timestamp = iTime.get_timestamp();
        size_t indx = 0;
        for(size_t i = 0; i < MINUTES_IN_DAY; ++i) {
            if(indx >= candles.size()) {
                write_null_candle_u32_5x(file);
            } else
            if(candles[indx].timestamp == timestamp) {
                write_candle_u32_5x(file, candles[i], i);
                indx++;

            } else
            if(candles[indx].timestamp > timestamp) {
                write_null_candle_u32_5x(file);
            }
            timestamp += xtime::SECONDS_IN_MINUTE;
        } // for i
        file.close();
        return OK;
    }

    /** \brief Читать бинарный файл котировок
     * \param file_name имя файла
     * \param start_timestamp начальная временная метка
     * \param prices котировки
     * \param times временные метки
     * \return вернет 0 в случае успешного завершения
     */
    int read_bin_file_u32_1x(std::string file_name,
            unsigned long long start_timestamp,
            std::vector<double> &prices,
            std::vector<unsigned long long> &times) {
        std::ifstream file(file_name, std::ios_base::binary);
        if(!file) return FILE_CANNOT_OPENED;
        for(int i = 0; i < MINUTES_IN_DAY; ++i) {
            prices.push_back(read_u32(file));
            times.push_back(start_timestamp);
            start_timestamp += xtime::SECONDS_IN_MINUTE;
        }
        file.close();
        return OK;
    }

    /** \brief Читать бинарный файл котировок
     * \param file_name имя файла
     * \param prices котировки
     * \return вернет 0 в случае успешного завершения
     */
    template <typename T>
    int read_bin_file_u32_1x(std::string file_name, T &prices) {
        std::ifstream file(file_name, std::ios_base::binary);
        if(!file) return FILE_CANNOT_OPENED;
        for(int i = 0; i < MINUTES_IN_DAY; ++i) {
            prices[i] = read_u32(file);
        }
        file.close();
        return OK;
    }

    /** \brief Читать бинарный файл котировок
     * \param file_name имя файла
     * \param timestamp временная метка начала дня
     * \param candles массив свечей
     * \return вернет 0 в случае успешного завершения
     */
    template <typename T>
    int read_bin_file_u32_4x(const std::string file_name, unsigned long long timestamp, T &candles) {
        std::ifstream file(file_name, std::ios_base::binary);
        if(!file) return FILE_CANNOT_OPENED;
        for(int i = 0; i < MINUTES_IN_DAY; ++i) {
            candles[i].open = read_u32(file);
            candles[i].high = read_u32(file);
            candles[i].low = read_u32(file);
            candles[i].close = read_u32(file);
            candles[i].timestamp = timestamp;
            timestamp += xtime::SECONDS_IN_MINUTE;
        }
        file.close();
        return OK;
    }

    /** \brief Читать бинарный файл котировок
     * \param file_name имя файла
     * \param timestamp временная метка начала дня
     * \param candles массив свечей
     * \return вернет 0 в случае успешного завершения
     */
    template <typename T>
    int read_bin_file_u32_5x(const std::string file_name, unsigned long long timestamp, T &candles) {
        std::ifstream file(file_name, std::ios_base::binary);
        if(!file) return FILE_CANNOT_OPENED;
        for(int i = 0; i < MINUTES_IN_DAY; ++i) {
            candles[i].open = read_u32(file);
            candles[i].high = read_u32(file);
            candles[i].low = read_u32(file);
            candles[i].close = read_u32(file);
            candles[i].volume = read_u32(file);
            candles[i].timestamp = timestamp;
            timestamp += xtime::SECONDS_IN_MINUTE;
        }
        file.close();
        return OK;
    }

    /** \brief Найти первую и последнюю дату файлов
     * \param file_list список файлов
     * \param file_extension расширение файла (например .hex или .zstd)
     * \param beg_timestamp первая дата, встречающееся среди файлов
     * \param end_timestamp последняя дата, встречающееся среди файлов
     * \return вернет 0 в случае успешного завершения
     */
    int get_beg_end_timestamp(
            std::vector<std::string> &file_list,
            std::string file_extension,
            unsigned long long &beg_timestamp,
            unsigned long long &end_timestamp) {
        if(file_list.size() == 0) return INVALID_PARAMETER;
        beg_timestamp = std::numeric_limits<unsigned long long>::max();
        end_timestamp = std::numeric_limits<unsigned long long>::min();
        for(size_t i = 0; i < file_list.size(); i++) {
            std::vector<std::string> path_file;
            bf::parse_path(file_list[i], path_file);
            if(path_file.size() == 0) continue;
            std::string file_name = path_file.back();
            // очищаем слово от расширения
                std::size_t first_pos = file_name.find(file_extension);
                if(first_pos == std::string::npos)
                continue;
                unsigned long long time;
                std::string word = file_name.substr(0, first_pos);
                if(xtime::convert_str_to_timestamp(file_name.substr(0, first_pos), time)) {
                    if(beg_timestamp > time) beg_timestamp = time;
                    if(end_timestamp < time) end_timestamp = time;
                }
        }
        if(beg_timestamp == std::numeric_limits<unsigned long long>::max() ||
            end_timestamp == std::numeric_limits<unsigned long long>::min()) return DATA_NOT_AVAILABLE;
        return OK;
    }

    /** \brief Найти первую и последнюю дату файлов
     * \param path директория с файлами исторических данных
     * \param file_extension расширение файла (например .hex или .zstd)
     * \param beg_timestamp первая дата, встречающееся среди файлов
     * \param end_timestamp последняя дата, встречающееся среди файлов
     * \return вернет 0 в случае успеха
     */
    int get_beg_end_timestamp_for_path(
            std::string path,
            std::string file_extension,
            unsigned long long &beg_timestamp,
            unsigned long long &end_timestamp) {
        std::vector<std::string> file_list;
        bf::get_list_files(path, file_list, true);
        return get_beg_end_timestamp(file_list, file_extension, beg_timestamp, end_timestamp);
    }

    /** \brief Найти первую и последнюю дату файлов в нескольких директориях
     * \param paths список директорий с файлами исторических данных
     * \param file_extension расширение файла (например .hex или .zstd)
     * \param beg_timestamp первая дата, встречающееся среди файлов
     * \param end_timestamp последняя дата, встречающееся среди файлов
     * \return вернет 0 в случае успеха
     */
    int get_beg_end_timestamp_for_paths(
            std::vector<std::string> paths,
            std::string file_extension,
            unsigned long long &beg_timestamp,
            unsigned long long &end_timestamp) {
        bool is_init = false;
        beg_timestamp = std::numeric_limits<unsigned long long>::min();
        end_timestamp = std::numeric_limits<unsigned long long>::max();
        for(size_t i = 0; i < paths.size(); ++i) {
            std::vector<std::string> file_list;
            bf::get_list_files(paths[i], file_list, true);
            unsigned long long _beg_timestamp;
            unsigned long long _end_timestamp;
            int err = get_beg_end_timestamp(file_list, file_extension, _beg_timestamp, _end_timestamp);
            if(err == OK) {
                if(beg_timestamp < _beg_timestamp) beg_timestamp = _beg_timestamp;
                if(end_timestamp > _end_timestamp) end_timestamp = _end_timestamp;
                is_init = true;
            }
        }
        if(!is_init) return DATA_NOT_AVAILABLE;
        return OK;
    }

    /** \brief Получить временную метку начала и конца данных для выгрузки
    * Данная функция вернет данные для выгрузки, начиная с вчерашнего дня или дня, который есть в исторических данных
    * За указаный перид. Т.е. по сути данная фукнция позволяет получить последние N дней.
    * \param path директория с файлами исторических данных
    * \param symbol имя валютной пары
    * \param file_extension расширение файла (например .hex или .zstd)
    * \param real_timestamp текущая временная метка
    * \param max_days количество дней
    * \param beg_timestamp первая дата, встречающееся среди файлов
    * \param end_timestamp последняя дата, встречающееся среди файлов
    * \return вернет 0 в случае успеха
    */
    int get_beg_end_timestamp(
            std::string path,
            std::string symbol,
            std::string file_extension,
            unsigned long long real_timestamp,
            int max_days,
            unsigned long long &beg_timestamp,
            unsigned long long &end_timestamp,
            bool is_use_day_off = false) {
        xtime::DateTime iRealTime(real_timestamp);
        iRealTime.set_beg_day();
        unsigned long long timestamp = iRealTime.get_timestamp() - xtime::SECONDS_IN_DAY;

        beg_timestamp = std::numeric_limits<unsigned long long>::max();
        end_timestamp = std::numeric_limits<unsigned long long>::min();
        int days = 0;
        int err_days = 0;
        const int MAX_ERR_DAYS = 30;
        while(true) {
            if(!is_use_day_off) { // пропускаем выходной день
                int wday = xtime::get_weekday(timestamp);
                if(wday == xtime::SUN || wday == xtime::SAT) {
                    timestamp -= xtime::SECONDS_IN_DAY;
                    continue;
                }
            }
            std::string name = path + "\\" + symbol + "\\" + get_file_name_from_date(timestamp);
            //std::cout << "name " << name << std::endl;
            if(bf::check_file(name + file_extension)) {
                days++;
                if(end_timestamp == std::numeric_limits<unsigned long long>::min()) {
                    end_timestamp = timestamp;
                } else if(days == max_days) {
                    beg_timestamp = timestamp;
                    return OK;
                }
                timestamp -= xtime::SECONDS_IN_DAY;
            } else {
                err_days++;
                if(err_days == MAX_ERR_DAYS) return DATA_NOT_AVAILABLE;
            }
        }
        return OK;
    }
}

#endif // BINARYAPIEASY_HPP_INCLUDED
