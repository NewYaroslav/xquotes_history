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
/** \file Файл для работы с csv форматом
 * Примеры варианты форматов csv:
 * 1) MT5 от Alpari:
 * <DATE>	<TIME>	<OPEN>	<HIGH>	<LOW>	<CLOSE>	<TICKVOL>	<VOL>	<SPREAD>
 * 2007.02.12	11:36:00	0.90510	0.90510	0.90500	0.90500	4	0	100
 * Длина строки 68
 * 2) Брокер dukascopy, исторические данные для скачивания с сайта:
 * Gmt time,Open,High,Low,Close,Volume
 * 01.01.2017 00:00:00.000,1150.312,1150.312,1150.312,1150.312,0
 * Длина строки 61 символ
 * 3) MT4 от Финам:
 * 1971.01.04,00:00,0.53690,0.53690,0.53690,0.53690,1
 * Длина строки 50
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

    enum {
        HEADER = -1,
        MT4 = 0,
        FINAM = 0,
        MT5 = 1,
        ALPARI = 1,
        DUKASCOPY = 2,
    };

    /** \brief Получить параметры из строки
     * \param line строка данных
     * \param timestamp метка времени
     * \param open цена открытия бара
     * \param high наивысшая цена бара
     * \param low наименьшая цена бара
     * \param close цена закрытия бара
     * \param volume объем бара
     * \return вернет true, если строка была правильно разобрана
     */
    bool parse_line(
            const std::string &line,
            unsigned long long &timestamp,
            double &open,
            double &high,
            double &low,
            double &close,
            double &volume) {
        std::size_t offset = 0;
        const std::string separator = ";,\t ";
        std::size_t found = line.find_first_of(separator, offset);

        if(found == std::string::npos) return false;
        std::string str_date = line.substr(offset, found - offset);
        if(str_date.size() == 10) {
            // варианты даты 2018.05.22 01.01.2017 1971.01.04
            offset = found + 1;
            found = line.find_first_of(separator, offset);
            std::string str_time = line.substr(offset, found - offset);

            std::size_t found_point = str_date.find_first_of(".", 0);
            if(found_point == std::string::npos) return false;

            int seconds = 0;
            if(str_time.size() >= 8) {
                // пример 00:00:00.000
                seconds = atoi((str_time.substr(6, 2)).c_str());
            }
            if(found_point == 4) {
               timestamp =  xtime::get_timestamp(
                    atoi((str_date.substr(8, 2)).c_str()),
                    atoi((str_date.substr(5, 2)).c_str()),
                    atoi((str_date.substr(0, 4)).c_str()),
                    atoi((str_time.substr(0, 2)).c_str()),
                    atoi((str_time.substr(3, 2)).c_str()),
                    seconds);
            } else
            if(found_point == 2) {
                timestamp =  xtime::get_timestamp(
                    atoi((str_date.substr(0, 2)).c_str()), // день
                    atoi((str_date.substr(3, 2)).c_str()), // месяц
                    atoi((str_date.substr(6, 4)).c_str()), // год
                    atoi((str_time.substr(0, 2)).c_str()),
                    atoi((str_time.substr(3, 2)).c_str()),
                    seconds);
            }
        } else {
            std::cout << "error date time size: " << str_date.size() << std::endl;
            timestamp = 0;
            return false;
        }
        // price open
        offset = found + 1;
        found = line.find_first_of(separator, offset);
        if(found == std::string::npos) return false;
        open = atof((line.substr(offset, found - offset)).c_str());
        // price high
        offset = found + 1;
        found = line.find_first_of(separator, offset);
        if(found == std::string::npos) return false;
        high = atof((line.substr(offset, found - offset)).c_str());
        // price low
        offset = found + 1;
        found = line.find_first_of(separator, offset);
        if(found == std::string::npos) return false;
        low = atof((line.substr(offset, found - offset)).c_str());
        // price close
        offset = found + 1;
        found = line.find_first_of(separator, offset);
        if(found == std::string::npos) return false;
        close = atof((line.substr(offset, found - offset)).c_str());
        volume = atof((line.substr(found + 1)).c_str());

        //std::cout << open << " " << high << " " << low << " " << close << " " << volume << std::endl;
        return true;
    }

    /** \brief Прочитать файл
     * \param file_name
     * \param is_read_header
     * \param time_zone
     * \param f
     * \return
     */
    int read_file(
            std::string file_name,
            bool is_read_header,
            int time_zone,
            std::function<void (const Candle candle, const bool is_end)> f) {
        std::ifstream file(file_name);
        if(!file.is_open()) {
            return FILE_CANNOT_OPENED;
        }
        std::string buffer;

        // получаем заголовок файла
        if(is_read_header) std::getline(file, buffer);

        while(!file.eof()) {
            std::getline(file, buffer);
            unsigned long long timestamp;
            double open, high, low, close, volume;
            if(parse_line(buffer, timestamp, open, high, low, close, volume)) {
                if(time_zone == CET_TO_GMT) timestamp = xtime::convert_cet_to_gmt(timestamp);
                else if(time_zone == EET_TO_GMT) timestamp = xtime::convert_eet_to_gmt(timestamp);
                else if(time_zone == GMT_TO_CET) timestamp = xtime::convert_gmt_to_cet(timestamp);
                else if(time_zone == GMT_TO_EET) timestamp = xtime::convert_gmt_to_eet(timestamp);
                f(Candle(open, high, low, close, volume, timestamp), false);
            }
        }
        f(Candle(), true);
        file.close();
        return OK;
    }

    /** \brief Проверка свечи или бара
     * \param candle свеча
     * \return вернет true если данные по бару есть
     */
    bool validation_candles(Candle &candle) {
        if(candle.close == 0 || candle.open == 0 || candle.low == 0 || candle.high == 0) {
            double price =
                candle.low != 0 ?
                candle.low : candle.high != 0 ?
                candle.high : candle.open != 0 ?
                candle.open : candle.close != 0 ?
                candle.close : 0.0;
            if(price == 0.0) return false;
            candle.open = price;
            candle.high = price;
            candle.low = price;
            candle.close = price;
        }
        return true;
    }

    /** \brief Записать файл
     * \param file_name Имя csv файла, куда запишем данные
     * \param header Заголовок csv файла
     * \param is_write_header Флаг записи заголовка csv файла. Если true, заголовок будет записан
     * \param start_timestamp Метка времени начала записи. Метка времени должна находится в том же часовом поясе, в котором находится источник данных цен!
     * \param stop_timestamp Метка времени завершения записи. Данная метка времени будет включена в массив цен!
     * \param type_csv Тип csv файла (MT4, MT5, DUKASCOPY)
     * \param type_correction_candle Тип коррекции бара или свечи
     * Варианты параметра: (SKIPPING_BAD_CANDLES, FILLING_BAD_CANDLES, WRITE_BAD_CANDLES)
     * При выборе FILLING_BAD_CANDLES бары или свечи, данные которых отсутствуют, будут заполнены
     * предыдущими значениями или пропущены, если предыдущих значений нет
     * \param time_zone Изменить часовой пояс меток времени
     * (DO_NOT_CHANGE_TIME_ZONE, CET_TO_GMT, EET_TO_GMT, GMT_TO_CET, GMT_TO_EET)
     * \param number_decimal_places количество знаков после запятой
     * \param f лямбда-функция для получения бара или свечи по метке времени, должна вернуть false в случае завершения
     * Лямбда функция может пропускать запись по своему усмотрению. Для этого достаточно вернуть false.
     * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
     */
    int write_file(
            const std::string file_name,
            const std::string header,
            bool is_write_header,
            const xtime::timestamp_t start_timestamp,
            const xtime::timestamp_t stop_timestamp,
            const int type_csv,
            const int type_correction_candle,
            const int time_zone,
            const int number_decimal_places,
            std::function<bool(Candle &candle, const xtime::timestamp_t timestamp)> f) {
        std::ofstream file(file_name);
        if(!file.is_open()) {
            return FILE_CANNOT_OPENED;
        }
        std::string str_price = "%." + std::to_string(number_decimal_places) + "f";
        std::string sprintf_param =
            // пример MT4: 1971.01.04,00:00,0.53690,0.53690,0.53690,0.53690,1
            type_csv == MT4 ? "%.4d.%.2d.%.2d,%.2d:%.2d," + str_price + "," + str_price + "," + str_price + "," + str_price + ",%d" :
            /* пример MT5: 2007.02.12	11:36:00	0.90510	0.90510	0.90500	0.90500	4	0	100
             * спред и реальный объем придется заполнить 0
             */
            type_csv == MT5 ? "%.4d.%.2d.%.2d\t%.2d:%.2d:%.2d\t" + str_price + "\t" + str_price + "\t" + str_price + "\t" + str_price + "\t" + "%d\t0\t0" :
            // пример DUKASCOPY: 01.01.2017 00:00:00.000,1150.312,1150.312,1150.312,1150.312,0
            type_csv == DUKASCOPY ? "%.2d.%.2d.%.4d %.2d:%.2d:%.2d.000," + str_price + "," + str_price + "," + str_price + "," + str_price + "," + "%f" :
            "%.4d.%.2d.%.2d,%.2d:%.2d," + str_price + "," + str_price + "," + str_price + "," + str_price + ",%d";

        if(is_write_header) file << header << std::endl;

        xtime::timestamp_t timestamp = start_timestamp;
        Candle old_candle;
        const int BUFFER_SIZE = 1024;
        char buffer[BUFFER_SIZE];

        while(true) {
            Candle candle;
            if(!f(candle, timestamp)) {
                timestamp += xtime::SECONDS_IN_MINUTE;
                if(timestamp > stop_timestamp) break;
                continue;
            }

            if(type_correction_candle == SKIPPING_BAD_CANDLES && !validation_candles(candle)) {
                timestamp += xtime::SECONDS_IN_MINUTE;
                if(timestamp > stop_timestamp) break;
                continue;
            } else
            if(type_correction_candle == FILLING_BAD_CANDLES && !validation_candles(candle)) {
                if(!validation_candles(old_candle)) {
                    timestamp += xtime::SECONDS_IN_MINUTE;
                    if(timestamp > stop_timestamp) break;
                    continue;
                }
                candle = old_candle;
                candle.timestamp = timestamp;
            } else {
                old_candle = candle;
            }

            xtime::timestamp_t t = time_zone == CET_TO_GMT ?
                xtime::convert_cet_to_gmt(candle.timestamp) : time_zone == EET_TO_GMT ?
                xtime::convert_eet_to_gmt(candle.timestamp) : time_zone == GMT_TO_CET ?
                xtime::convert_gmt_to_cet(candle.timestamp) : time_zone == GMT_TO_EET ?
                xtime::convert_gmt_to_eet(candle.timestamp) : candle.timestamp;
            xtime::DateTime date_time(t);
            std::fill(buffer, buffer + BUFFER_SIZE, '\0');
            switch(type_csv) {
            case MT4:
            default:
                sprintf(
                    buffer,
                    sprintf_param.c_str(),
                    date_time.year,
                    date_time.month,
                    date_time.day,
                    date_time.hour,
                    date_time.minutes,
                    candle.open,
                    candle.high,
                    candle.low,
                    candle.close,
                    (int)candle.volume);
                break;
            case MT5:
                sprintf(
                    buffer,
                    sprintf_param.c_str(),
                    date_time.year,
                    date_time.month,
                    date_time.day,
                    date_time.hour,
                    date_time.minutes,
                    date_time.seconds,
                    candle.open,
                    candle.high,
                    candle.low,
                    candle.close,
                    (int)candle.volume);
                break;
            case DUKASCOPY:
                sprintf(
                    buffer,
                    sprintf_param.c_str(),
                    date_time.day,
                    date_time.month,
                    date_time.year,
                    date_time.hour,
                    date_time.minutes,
                    date_time.seconds,
                    candle.open,
                    candle.high,
                    candle.low,
                    candle.close,
                    candle.volume);
                break;
            }
            std::string line(buffer);
            file << line << std::endl;
            timestamp += xtime::SECONDS_IN_MINUTE;
            if(timestamp > stop_timestamp) break;
        }
        file.close();
        return OK;
    }
}
#endif // XQUOTES_CSV_HPP_INCLUDED
