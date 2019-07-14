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
#ifndef BINARYAPIEASY_HPP_INCLUDED
#define BINARYAPIEASY_HPP_INCLUDED

#include "xquotes_common.hpp"
#include "banana_filesystem.hpp"
#include "xtime.hpp"
#include <limits>

namespace xquotes_history {
    using namespace xquotes_common;

    /** \brief Перевести double-цену в формат uint32_t
     * \param price цена
     * \return цена в формате uint32_t
     */
    inline unsigned long convert_to_int(double price) {
        return (unsigned long)((price * PRICE_MULTIPLER) + 0.5);
    }

    /** \brief Перевести uint32_t-цену в формат double
     * \param price цена
     * \return цена в формате uint32_t
     */
    inline double convert_to_double(unsigned long price) {
        return (double)price / PRICE_MULTIPLER;
    }

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

    /** \brief Записать бинарный файл котировок
     * \param file_name имя файла
     * \param prices котировки
     * \return вернет 0 в случае успешного завершения
     */
    template <typename T>
    int write_binary_file(
            std::string file_name,
            T &prices) {
        if(prices.size() != MINUTES_IN_DAY) return INVALID_ARRAY_LENGH;
        std::ofstream file(file_name, std::ios_base::binary);
        for(int i = 0; i < MINUTES_IN_DAY; ++i) {
            unsigned long value = convert_to_int(prices[i]);
            file.write(reinterpret_cast<char *>(&value), sizeof(unsigned long));
        }
        file.close();
        return OK;
    }

    /** \brief Записать бинарный файл котировок
     * Данная функция может принимать котировки с пропусками временых меток
     * Еще данная функция
     * \param file_name имя файла
     * \param prices котировки
     * \param times временные метки
     * \return вернет 0 в случае успешного завершения
     */
    int write_binary_file(std::string file_name,
            std::vector<double> &prices,
            std::vector<unsigned long long> &times) {
        if(prices.size() != times.size() || prices.size() == 0) return INVALID_ARRAY_LENGH;
        std::ofstream file(file_name, std::ios_base::binary);
        xtime::DateTime iTime(times[0]);
        iTime.set_beg_day();
        unsigned long long timestamp = iTime.get_timestamp();
        int times_indx = 0;
        for(int i = 0; i < MINUTES_IN_DAY; ++i) {
            const unsigned long PRICE_NULL = 0;
            if(times_indx >= times.size()) {
                file.write(const_cast<char* >(reinterpret_cast<const char *>(&PRICE_NULL)), sizeof(unsigned long));
            } else
            if(times[times_indx] == timestamp) {
                unsigned long value = convert_to_int(prices[times_indx]);
                file.write(reinterpret_cast<char *>(&value), sizeof(unsigned long));
                times_indx++;
            } else
            if(times[times_indx] > timestamp) {
                file.write(const_cast<char* >(reinterpret_cast<const char *>(&PRICE_NULL)), sizeof(unsigned long));
            }
            timestamp += xtime::SECONDS_IN_MINUTE;
        }
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
    int read_binary_file(std::string file_name,
            unsigned long long start_timestamp,
            std::vector<double> &prices,
            std::vector<unsigned long long> &times) {
        std::ifstream file(file_name, std::ios_base::binary);
        if(!file) return FILE_CANNOT_OPENED;
        for(int i = 0; i < MINUTES_IN_DAY; ++i) {
            unsigned long value = 0;
            file.read(reinterpret_cast<char *>(&value),sizeof (unsigned long));
            if(value != 0) {
                prices.push_back(convert_to_double(value));
                times.push_back(start_timestamp);
            }
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
    int read_binary_file(std::string file_name,
            T &prices) {
        std::ifstream file(file_name, std::ios_base::binary);
        if(!file) return FILE_CANNOT_OPENED;
        for(int i = 0; i < MINUTES_IN_DAY; ++i) {
            unsigned long value = 0;
            file.read(reinterpret_cast<char *>(&value),sizeof (unsigned long));
            prices[i] = convert_to_double(value);
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
//------------------------------------------------------------------------------
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
