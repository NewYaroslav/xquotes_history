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

/** \file Файл с классами для работы с историческими данными котировок
 * \brief Данный файл содержит два класса - QuotesHistory и MultipleQuotesHistory
 * Класс QuotesHistory предназначен для работы с одной валютной парой, в то время как
 * MultipleQuotesHistory может работать сразу с несколькими
 */
#ifndef XQUOTES_HISTORY_HPP_INCLUDED
#define XQUOTES_HISTORY_HPP_INCLUDED

#include "xquotes_storage.hpp"
#include <array>
#include <functional>
#ifndef XQUOTES_DO_NOT_USE_THREAD
#include <thread>
#include <mutex>
#endif

// подключаем словари для сжатия файлов
#include "xquotes_dictionary_candles.hpp"
#include "xquotes_dictionary_candles_with_volumes.hpp"
#include "xquotes_dictionary_only_one_price.hpp"

#ifdef XQUOTES_USE_DICTIONARY_CURRENCY_PAIR
// словари под конкретные валютные пары
#include "dictionary_currency_pair/xquotes_dictionary_candles_audcad.hpp"
#include "dictionary_currency_pair/xquotes_dictionary_candles_audchf.hpp"
#include "dictionary_currency_pair/xquotes_dictionary_candles_audjpy.hpp"
#include "dictionary_currency_pair/xquotes_dictionary_candles_audnzd.hpp"
#include "dictionary_currency_pair/xquotes_dictionary_candles_audusd.hpp"
#include "dictionary_currency_pair/xquotes_dictionary_candles_cadchf.hpp"
#include "dictionary_currency_pair/xquotes_dictionary_candles_cadjpy.hpp"
#include "dictionary_currency_pair/xquotes_dictionary_candles_chfjpy.hpp"
#include "dictionary_currency_pair/xquotes_dictionary_candles_euraud.hpp"
#include "dictionary_currency_pair/xquotes_dictionary_candles_eurcad.hpp"
#include "dictionary_currency_pair/xquotes_dictionary_candles_eurchf.hpp"
#include "dictionary_currency_pair/xquotes_dictionary_candles_eurgbp.hpp"
#include "dictionary_currency_pair/xquotes_dictionary_candles_eurjpy.hpp"
#include "dictionary_currency_pair/xquotes_dictionary_candles_eurnok.hpp"
#include "dictionary_currency_pair/xquotes_dictionary_candles_eurnzd.hpp"
#include "dictionary_currency_pair/xquotes_dictionary_candles_eurusd.hpp"
#include "dictionary_currency_pair/xquotes_dictionary_candles_gbpaud.hpp"
#include "dictionary_currency_pair/xquotes_dictionary_candles_gbpcad.hpp"
#include "dictionary_currency_pair/xquotes_dictionary_candles_gbpchf.hpp"
#include "dictionary_currency_pair/xquotes_dictionary_candles_gbpjpy.hpp"
#include "dictionary_currency_pair/xquotes_dictionary_candles_gbpnok.hpp"
#include "dictionary_currency_pair/xquotes_dictionary_candles_gbpnzd.hpp"
#include "dictionary_currency_pair/xquotes_dictionary_candles_gbpusd.hpp"
#include "dictionary_currency_pair/xquotes_dictionary_candles_nzdcad.hpp"
#include "dictionary_currency_pair/xquotes_dictionary_candles_nzdjpy.hpp"
#include "dictionary_currency_pair/xquotes_dictionary_candles_nzdusd.hpp"
#include "dictionary_currency_pair/xquotes_dictionary_candles_usdcad.hpp"
#include "dictionary_currency_pair/xquotes_dictionary_candles_usdchf.hpp"
#include "dictionary_currency_pair/xquotes_dictionary_candles_usdjpy.hpp"
#include "dictionary_currency_pair/xquotes_dictionary_candles_usdnok.hpp"
#include "dictionary_currency_pair/xquotes_dictionary_candles_usdpln.hpp"
#endif // XQUOTES_USE_DICTIONARY_CURRENCY_PAIR

namespace xquotes_history {
    using namespace xquotes_common;
    using namespace xquotes_storage;
    using namespace xquotes_dictionary;

    /** \brief Класс для удобного использования исторических данных
     * Данный класс имеет оптимизированный для поминутного чтения данных метод - get_candle
     * Оптимизация метода get_candle заключается в том, что поиск следующей цены начинается только в случае,
     * если метка времени следующей цены не совпала с запрашиваемой меткой времени
     * Метод find_candle является по сути аналогом get_candle, но он всегда ищет цену
     */
    template <class CANDLE_TYPE = Candle>
    class QuotesHistory : public Storage {
    private:
        typedef std::array<CANDLE_TYPE, MINUTES_IN_DAY> candles_array_t;    /**< Массив свечей */
        bool is_use_dictionary = false; /**< Флаг использования словаря */
        int price_type = PRICE_CLOSE;   /**< Тип используемой в хранилище цены */
#       ifdef XQUOTES_USE_DICTIONARY_CURRENCY_PAIR
        int currency_pair = 0;          /**< Валютная пара */
#       endif
        int decimal_places_ = 0;        /**< Количество знаков после запятой */
        std::string path_;
        std::string name_;

        std::unique_ptr<char[]> write_buffer;   /**< Буфер для записи */
        size_t write_buffer_size = 0;           /**< Размер буфера для записи */

        /** \brief Изменить размер буффера для записи
         * Данная функция работает только на увеличение размера буффера
         * \param new_size новый размер
         */
        void increase_write_buffer_size(const size_t &new_size) {
            if(new_size > write_buffer_size) {
                write_buffer = std::unique_ptr<char[]>(new char[new_size]);
                write_buffer_size = new_size;
            }
        }

        std::unique_ptr<char[]> read_candles_buffer;    /**< Буфер для чтения свечей */
        size_t read_candles_buffer_size = 0;            /**< Размер буфера для чтения свечей */

        /** \brief Изменить размер буффера для чтения свечей
         * Данная функция работает только на увеличение размера буффера
         * \param new_size новый размер
         */
        void increase_read_candles_buffer_size(const size_t &new_size) {
            if(new_size > write_buffer_size) {
                read_candles_buffer = std::unique_ptr<char[]>(new char[new_size]);
                read_candles_buffer_size = new_size;
            }
        }

        std::vector<candles_array_t> candles_array_days; /**< Котировки в виде минутных свечей, отсортирован по дням */

        /** \brief Получить метку времени начала дня массива цен по индексу элемента
         * Данный метод нужен для внутреннего использования
         * \param ind индекс элемента массива цен
         * \return метка времени начала дня
         */
        inline ztime::timestamp_t get_prices_timestamp(const int &ind) {
            return candles_array_days[ind][0].timestamp;
        }

        /** \brief Сортировка массивов минутных свечей
         * Данный метод нужен для внутреннего использования
         */
        void sort_candles_array_days() {
            if(!std::is_sorted(candles_array_days.begin(), candles_array_days.end(), [](const candles_array_t &a, const candles_array_t &b) {
                        return a[0].timestamp < b[0].timestamp;
                    })) {
                std::sort(candles_array_days.begin(), candles_array_days.end(), [](const candles_array_t &a, const candles_array_t &b) {
                    return a[0].timestamp < b[0].timestamp;
                });;
            }
        }

        /** \brief Найти массив свечей за конкретный день
         * Метод ищет массив свечей конкретного дня по метке времени
         * Данный метод нужен для внутреннего использования
         * \param timestamp метка времени (должна быть всегда в начале дня!)
         * \return указатель на массив свечей или NULL, если даных нет в списке
         */
        typename std::vector<candles_array_t>::iterator find_candles_array(
                const ztime::timestamp_t &timestamp) {
            if(candles_array_days.size() == 0) return candles_array_days.end();
            auto prices_it = std::lower_bound(
                candles_array_days.begin(),
                candles_array_days.end(),
                timestamp,
                [](const candles_array_t &lhs, const ztime::timestamp_t &timestamp) {
                return lhs[0].timestamp < timestamp;
            });
            if(prices_it == candles_array_days.end()) {
                return candles_array_days.end();
            } else
            if(prices_it->at(0).timestamp == timestamp) {
                //return &(*prices_it);
                return prices_it;
            }
            return candles_array_days.end();
        }

        int indent_day_up = 1;          /**< Отступ в будущее в днях от текущей метки времени */
        int indent_day_dn = 1;          /**< Отступ в прошлое в днях от текущей метки времени */

        int ind_forecast_day = 0;      /**< Прогноз индекса (дня) в массиве candles_array_days */
        int ind_forecast_minute = 0;   /**< Прогноз индекса в массиве свечей candles_array_days[ind_forecast_day] */

        /** \brief Сделать следующий прогноз для идексов массива свечей
         * Данный метод нужен для внутреннего использования
         */
        inline void make_next_candles_forecast() {
            ind_forecast_minute++;
            if(ind_forecast_minute >= MINUTES_IN_DAY) {
                ind_forecast_minute = 0;
                if(ind_forecast_day >= ((int)candles_array_days.size() - 1)) return;
                ind_forecast_day++;
            }
        }

        /** \brief Установить начальное положение идексов массива цен
         * Данный метод нужен для внутреннего использования
         * \param timestamp временная метка (должна быть всегда в начале дня!)
         * \param minute_day минута дня
         * \return вернет true если цена была найдена
         */
        inline bool set_start_candles_forecast(
                const ztime::timestamp_t &timestamp,
                const int &minute_day) {
            auto found_candles_array = find_candles_array(timestamp);
            if(found_candles_array == candles_array_days.end()) return false;
            ind_forecast_day = found_candles_array - candles_array_days.begin();
            ind_forecast_minute = minute_day;
            return true;
        }

        /** \brief Заполнить метки времени массива свечей
         * Данный метод нужен для внутреннего использования
         */
        void fill_timestamp(
                std::array<CANDLE_TYPE, MINUTES_IN_DAY>& candles,
                const ztime::timestamp_t &timestamp) {
            candles[0].timestamp = timestamp;
            for(int i = 1; i < MINUTES_IN_DAY; ++i) {
                candles[i].timestamp = candles[i - 1].timestamp + ztime::SECONDS_IN_MINUTE;
            }
        }

        /** \brief Конвертировать массив свечей в буфер
         * Данный метод нужен для внутреннего использования
         */
        int convert_candles_to_buffer(
                const std::array<CANDLE_TYPE, MINUTES_IN_DAY>& candles,
                char* buffer,
                const unsigned long &buffer_size) {
            if(buffer_size == ONLY_ONE_PRICE_BUFFER_SIZE) {
                if(price_type == PRICE_CLOSE) {
                    for(int i = 0; i < MINUTES_IN_DAY; ++i) {
                        ((price_t*)buffer)[i] = convert_to_uint(candles[i].close);
                    }
                } else
                if(price_type == PRICE_OPEN) {
                    for(int i = 0; i < MINUTES_IN_DAY; ++i) {
                        ((price_t*)buffer)[i] = convert_to_uint(candles[i].open);
                    }
                }
            } else
            if(buffer_size == CANDLE_WITHOUT_VOLUME_BUFFER_SIZE) {
                const int BUFFER_SAMPLE_SIZE = 4;
                for(int i = 0; i < MINUTES_IN_DAY; ++i) {
                    int ind = i * BUFFER_SAMPLE_SIZE;
                    ((price_t*)buffer)[ind + 0] = convert_to_uint(candles[i].open);
                    ((price_t*)buffer)[ind + 1] = convert_to_uint(candles[i].high);
                    ((price_t*)buffer)[ind + 2] = convert_to_uint(candles[i].low);
                    ((price_t*)buffer)[ind + 3] = convert_to_uint(candles[i].close);
                }
            } else
            if(buffer_size == CANDLE_WITH_VOLUME_BUFFER_SIZE) {
                const int BUFFER_SAMPLE_SIZE = 5;
                for(int i = 0; i < MINUTES_IN_DAY; ++i) {
                    int ind = i * BUFFER_SAMPLE_SIZE;
                    ((price_t*)buffer)[ind + 0] = convert_to_uint(candles[i].open);
                    ((price_t*)buffer)[ind + 1] = convert_to_uint(candles[i].high);
                    ((price_t*)buffer)[ind + 2] = convert_to_uint(candles[i].low);
                    ((price_t*)buffer)[ind + 3] = convert_to_uint(candles[i].close);
                    ((price_t*)buffer)[ind + 4] = convert_to_uint(candles[i].volume);
                }
            } else {
                //std::cout << "buffer_size " << buffer_size << std::endl;
                return INVALID_ARRAY_LENGH;
            }
            return OK;
        }

        /** \brief Конвертировать буфер в массив свечей
         * Данный метод нужен для внутреннего использования
         */
        int convert_buffer_to_candles(
                std::array<CANDLE_TYPE, MINUTES_IN_DAY>& candles,
                const char *buffer,
                const unsigned long &buffer_size) {
            if(buffer_size == ONLY_ONE_PRICE_BUFFER_SIZE) {
                for(int i = 0; i < MINUTES_IN_DAY; ++i) {
                    candles[i].close = convert_to_double(((price_t*)buffer)[i]);
                }
            } else
            if(buffer_size == CANDLE_WITHOUT_VOLUME_BUFFER_SIZE) {
                const int BUFFER_SAMPLE_SIZE = 4;
                for(int i = 0; i < MINUTES_IN_DAY; ++i) {
                    int ind = i * BUFFER_SAMPLE_SIZE;
                    candles[i].open = convert_to_double(((price_t*)buffer)[ind + 0]);
                    candles[i].high = convert_to_double(((price_t*)buffer)[ind + 1]);
                    candles[i].low = convert_to_double(((price_t*)buffer)[ind + 2]);
                    candles[i].close = convert_to_double(((price_t*)buffer)[ind + 3]);
                }
            } else
            if(buffer_size == CANDLE_WITH_VOLUME_BUFFER_SIZE) {
                const int BUFFER_SAMPLE_SIZE = 5;
                for(int i = 0; i < MINUTES_IN_DAY; ++i) {
                    int ind = i * BUFFER_SAMPLE_SIZE;
                    candles[i].open = convert_to_double(((price_t*)buffer)[ind + 0]);
                    candles[i].high = convert_to_double(((price_t*)buffer)[ind + 1]);
                    candles[i].low = convert_to_double(((price_t*)buffer)[ind + 2]);
                    candles[i].close = convert_to_double(((price_t*)buffer)[ind + 3]);
                    candles[i].volume = convert_to_double(((price_t*)buffer)[ind + 4]);
                }
            } else {
                return INVALID_ARRAY_LENGH;
            }
            return OK;
        }

        /** \brief Прочитать свечи
         * \warning Данный метод нужен для внутреннего использования
         * \param candles массив свечей за день
         * \param key ключ, это день с начала unix времени
         * \param timestamp метка времени (должна быть всегда в начале дня!)
         * \return состояние ошибки
         */
        int read_candles(std::array<CANDLE_TYPE, MINUTES_IN_DAY>& candles, const key_t &key, const ztime::timestamp_t &timestamp) {
            int err = 0;
            unsigned long buffer_size = 0;
            fill_timestamp(candles, timestamp);
            if(is_use_dictionary) err = read_compressed_subfile(key, read_candles_buffer, read_candles_buffer_size, buffer_size);
            else err = read_subfile(key, read_candles_buffer, read_candles_buffer_size, buffer_size);
            if(err != OK) {
                return err;
            }
            err = convert_buffer_to_candles(candles, read_candles_buffer.get(), buffer_size);
            return err;
        }

        /** \brief Прочитать данные
         *
         * Данный метод нужен для внутреннего использования
         * \param timestamp временная метка требуемого дня котировоки (это обязательно начало дня!)
         * \param indent_dn отступ от временная метки в днях на уменьшение времени
         * \param indent_up отступ от временная метки в днях на увеличение времени
         */
        void read_candles_data(const ztime::timestamp_t &timestamp, const int &indent_dn, const int &indent_up) {
            int num_days = indent_dn + indent_up + 1;
            ztime::timestamp_t start_ind_timestamp = timestamp - indent_dn * ztime::SECONDS_IN_DAY;
            int start_ind_day = ztime::get_day(start_ind_timestamp);
            if(candles_array_days.size() == 0) {
                // в данном случае просто загружаем данные
                candles_array_days.resize(num_days);
                int ind_day = start_ind_day;
                ztime::timestamp_t ind_timestamp = start_ind_timestamp;
                for(int i = 0; i < num_days; ++i, ++ind_day, ind_timestamp += ztime::SECONDS_IN_DAY) {
                    read_candles(candles_array_days[i], ind_day, ind_timestamp);
                }
            } else {
                // сначала составим список уже загруженных данных
                std::vector<ztime::timestamp_t> check_timestamp;
                for(size_t j = 0; j < candles_array_days.size(); ++j) {
                    //int ind_day = start_ind_day;
                    ztime::timestamp_t ind_timestamp = start_ind_timestamp;
                    for(int i = 0; i < num_days; ++i, ind_timestamp += ztime::SECONDS_IN_DAY) {
                    //for(int i = 0; i < num_days; ++i, ++ind_day, ind_timestamp += ztime::SECONDS_IN_DAY) {
                        if(std::binary_search(check_timestamp.begin(), check_timestamp.end(), ind_timestamp)) continue;
                        if(get_prices_timestamp(j) == ind_timestamp) {
                            auto check_timestamp_it = std::upper_bound(check_timestamp.begin(),check_timestamp.end(), ind_timestamp);
                            check_timestamp.insert(check_timestamp_it, ind_timestamp);
                        }
                    }
                }
                // удаляем лишние данные
                size_t ind_prices = 0;
                while(ind_prices < candles_array_days.size()) {
                    if(!std::binary_search(check_timestamp.begin(), check_timestamp.end(), candles_array_days[ind_prices][0].timestamp)) {
                        candles_array_days.erase(candles_array_days.begin() + ind_prices);
                        continue;
                    }
                    ind_prices++;
                }
                // далее загрузим недостающие данные
                int ind_day = start_ind_day;
                ztime::timestamp_t ind_timestamp = start_ind_timestamp;
                for(int i = 0; i < num_days; ++i, ++ind_day, ind_timestamp += ztime::SECONDS_IN_DAY) {
                    if(std::binary_search(check_timestamp.begin(), check_timestamp.end(), ind_timestamp)) continue;
                    candles_array_days.resize(candles_array_days.size() + 1);
                    read_candles(candles_array_days[candles_array_days.size() - 1], ind_day, ind_timestamp);
                }
            }
            sort_candles_array_days();
        }

        /** \brief Найти свечу по временной метке
         * \warning Данный метод нужен для внутреннего использования
         * \param candle Свеча/бар
         * \param timestamp временная метка начала свечи
         * \param optimization оптимизация.
         * Для отключения указать WITHOUT_OPTIMIZATION.
         * По умолчанию включена оптимизация последовательного считывания минут OPTIMIZATION_SEQUENTIAL_READING
         * \return вернет 0 в случае успеха
         */
        template <typename T>
        int find_candle(T &candle, const ztime::timestamp_t &timestamp) {
            int minute_day = ztime::get_minute_day(timestamp);
            const ztime::timestamp_t timestamp_start_day = ztime::get_first_timestamp_day(timestamp);
            auto found_candles_array = find_candles_array(timestamp_start_day);
            if(found_candles_array == candles_array_days.end()) {
                read_candles_data(timestamp_start_day, indent_day_dn, indent_day_up);
                found_candles_array = find_candles_array(timestamp_start_day);
                if(found_candles_array == candles_array_days.end()) return STRANGE_PROGRAM_BEHAVIOR;
            }
            int ind_day = found_candles_array - candles_array_days.begin();
            candle = candles_array_days[ind_day][minute_day];
            return candle.close != 0.0 ? OK : DATA_NOT_AVAILABLE;
        }

        /** \brief Обновить заметку файла
         * Данная функция проверяет, были ли записаны подфалы в файл
         * Если подфайлов нет, то заметку формируем из настроек пользователя
         * Если подфайлы есть, то заметку считываем из файла и игнорируем настройки пользователя
         * \param user_price_type пользовательная настройка цены
         */
        void update_file_notes(const int &user_price_type) {

#           ifdef XQUOTES_USE_DICTIONARY_CURRENCY_PAIR
            const unsigned int PRICE_TYPE_NOTES_MASK = 0x0F;
            const unsigned int COMPRESSION_BIT = 0x10;
            const unsigned int PRICE_TYPE_PAIR_MASK = 0xFF00;
            if(get_num_subfiles() == 0) {
                note_t notes = is_use_dictionary ? COMPRESSION_BIT : 0x00;
                notes |= user_price_type & PRICE_TYPE_NOTES_MASK;
                notes |= (user_price_type & PRICE_TYPE_PAIR_MASK);
                set_file_note(notes);
                QuotesHistory::price_type = user_price_type & PRICE_TYPE_NOTES_MASK;
                QuotesHistory::currency_pair = (user_price_type & PRICE_TYPE_PAIR_MASK) >> 8;
            } else {
                note_t notes = get_file_note();
                QuotesHistory::price_type = notes & PRICE_TYPE_NOTES_MASK;
                QuotesHistory::currency_pair = (notes & PRICE_TYPE_PAIR_MASK) >> 8;
                is_use_dictionary = notes & COMPRESSION_BIT ? true : false;
            }
#           else

            const unsigned int PRICE_TYPE_NOTES_MASK = 0x0F;
            const unsigned int COMPRESSION_BIT = 0x10;
            if(get_num_subfiles() == 0) {
                note_t notes = is_use_dictionary ? COMPRESSION_BIT : 0x00;
                notes |= user_price_type & PRICE_TYPE_NOTES_MASK;
                set_file_note(notes);
                QuotesHistory::price_type = user_price_type & PRICE_TYPE_NOTES_MASK;
            } else {
                note_t notes = get_file_note();
                QuotesHistory::price_type = notes & PRICE_TYPE_NOTES_MASK;
                is_use_dictionary = notes & COMPRESSION_BIT ? true : false;
            }
#           endif
        }

        /** \brief Проверка выходного дня
         */
        static bool check_day_off(const key_t &key) {
            return ztime::is_day_off_for_day(key);
        }

        public:

        QuotesHistory() : Storage() {};

        /** \brief Инициализировать класс
         * \param path директория с файлами исторических данных
         * \param user_price_type тип цены (на выбор: PRICE_CLOSE, PRICE_OHLC, PRICE_OHLCV)
         * \param dictionary_file файл словаря (если указано "", то считываются несжатые файлы)
         */
        QuotesHistory(
                const std::string &path,
                const int &user_price_type,
                const std::string &dictionary_file = "") :
                Storage(path, dictionary_file), path_(path) {
            std::vector<std::string> element;
            bf::parse_path(path, element);
            name_ = element.back();
            if(dictionary_file.size() > 0) is_use_dictionary = true;
            update_file_notes(user_price_type);
        }

        /** \brief Инициализировать класс
         * \param path путь к файлу с данными
         * \param user_price_type тип цены (на выбор: PRICE_CLOSE, PRICE_OHLC, PRICE_OHLCV)
         * \param dictionary_buffer указатель на буфер словаря
         * \param dictionary_buffer_size размер буфера словаря
         */
        QuotesHistory(
                const std::string &path,
                const int &user_price_type,
                const char *dictionary_buffer,
                const size_t &dictionary_buffer_size) :
                Storage(path, dictionary_buffer, dictionary_buffer_size), path_(path) {
            std::vector<std::string> element;
            bf::parse_path(path, element);
            name_ = element.back();
            is_use_dictionary = true;
            update_file_notes(user_price_type);
        }

        /** \brief Инициализировать класс
         * \param path путь к файлу с данными
         * \param price_type тип цены (на выбор: PRICE_CLOSE, PRICE_OHLC, PRICE_OHLCV или PRICE_OHLC_AUDCAD и пр..)
         * \param option настройки хранилища котировок (использовать сжатие  - USE_COMPRESSION, иначе DO_NOT_USE_COMPRESSION)
         */
        QuotesHistory(
                const std::string &path,
                const int &user_price_type,
                const int &option) :
                Storage(path), path_(path) {
            if(option == USE_COMPRESSION) is_use_dictionary = true;
            update_file_notes(user_price_type);
            if(is_use_dictionary) {
                switch (QuotesHistory::price_type){
                case PRICE_CLOSE:
                case PRICE_OPEN:
                    set_dictionary((const char*)dictionary_only_one_price, sizeof(dictionary_only_one_price));
                    break;
                case PRICE_OHLC:

#                   ifdef XQUOTES_USE_DICTIONARY_CURRENCY_PAIR
                    switch (QuotesHistory::currency_pair){
                    default:
                    case 0:
                        set_dictionary((const char*)dictionary_candles, sizeof(dictionary_candles));
                        break;
                    case USE_DICTIONARY_AUDCAD:
                        set_dictionary((const char*)dictionary_candles_audcad, sizeof(dictionary_candles_audcad));
                        break;
                    case USE_DICTIONARY_AUDCHF:
                        set_dictionary((const char*)dictionary_candles_audchf, sizeof(dictionary_candles_audchf));
                        break;
                    case USE_DICTIONARY_AUDJPY:
                        set_dictionary((const char*)dictionary_candles_audjpy, sizeof(dictionary_candles_audjpy));
                        break;
                    case USE_DICTIONARY_AUDNZD:
                        set_dictionary((const char*)dictionary_candles_audnzd, sizeof(dictionary_candles_audnzd));
                        break;
                    case USE_DICTIONARY_AUDUSD:
                        set_dictionary((const char*)dictionary_candles_audusd, sizeof(dictionary_candles_audusd));
                        break;
                    case USE_DICTIONARY_CADCHF:
                        set_dictionary((const char*)dictionary_candles_cadchf, sizeof(dictionary_candles_cadchf));
                        break;
                    case USE_DICTIONARY_CADJPY:
                        set_dictionary((const char*)dictionary_candles_cadjpy, sizeof(dictionary_candles_cadjpy));
                        break;
                    case USE_DICTIONARY_CHFJPY:
                        set_dictionary((const char*)dictionary_candles_chfjpy, sizeof(dictionary_candles_chfjpy));
                        break;
                    case USE_DICTIONARY_EURAUD:
                        set_dictionary((const char*)dictionary_candles_euraud, sizeof(dictionary_candles_euraud));
                        break;
                    case USE_DICTIONARY_EURCAD:
                        set_dictionary((const char*)dictionary_candles_eurcad, sizeof(dictionary_candles_eurcad));
                        break;
                    case USE_DICTIONARY_EURCHF:
                        set_dictionary((const char*)dictionary_candles_eurchf, sizeof(dictionary_candles_eurchf));
                        break;
                    case USE_DICTIONARY_EURGBP:
                        set_dictionary((const char*)dictionary_candles_eurgbp, sizeof(dictionary_candles_eurgbp));
                        break;
                    case USE_DICTIONARY_EURJPY:
                        set_dictionary((const char*)dictionary_candles_eurjpy, sizeof(dictionary_candles_eurjpy));
                        break;
                    case USE_DICTIONARY_EURNOK:
                        set_dictionary((const char*)dictionary_candles_eurnok, sizeof(dictionary_candles_eurnok));
                        break;
                    case USE_DICTIONARY_EURUSD:
                        set_dictionary((const char*)dictionary_candles_eurusd, sizeof(dictionary_candles_eurusd));
                        break;
                    case USE_DICTIONARY_GBPAUD:
                        set_dictionary((const char*)dictionary_candles_gbpaud, sizeof(dictionary_candles_gbpaud));
                        break;
                    case USE_DICTIONARY_GBPCAD:
                        set_dictionary((const char*)dictionary_candles_gbpcad, sizeof(dictionary_candles_gbpcad));
                        break;
                    case USE_DICTIONARY_GBPCHF:
                        set_dictionary((const char*)dictionary_candles_gbpchf, sizeof(dictionary_candles_gbpchf));
                        break;
                    case USE_DICTIONARY_GBPJPY:
                        set_dictionary((const char*)dictionary_candles_gbpjpy, sizeof(dictionary_candles_gbpjpy));
                        break;
                    case USE_DICTIONARY_GBPNOK:
                        set_dictionary((const char*)dictionary_candles_gbpnok, sizeof(dictionary_candles_gbpnok));
                        break;
                    case USE_DICTIONARY_GBPNZD:
                        set_dictionary((const char*)dictionary_candles_gbpnzd, sizeof(dictionary_candles_gbpnzd));
                        break;
                    case USE_DICTIONARY_GBPUSD:
                        set_dictionary((const char*)dictionary_candles_gbpusd, sizeof(dictionary_candles_gbpusd));
                        break;
                    case USE_DICTIONARY_NZDCAD:
                        set_dictionary((const char*)dictionary_candles_nzdcad, sizeof(dictionary_candles_nzdcad));
                        break;
                    case USE_DICTIONARY_NZDJPY:
                        set_dictionary((const char*)dictionary_candles_nzdjpy, sizeof(dictionary_candles_nzdjpy));
                        break;
                    case USE_DICTIONARY_NZDUSD:
                        set_dictionary((const char*)dictionary_candles_nzdusd, sizeof(dictionary_candles_nzdusd));
                        break;
                    case USE_DICTIONARY_USDCAD:
                        set_dictionary((const char*)dictionary_candles_usdcad, sizeof(dictionary_candles_usdcad));
                        break;
                    case USE_DICTIONARY_USDCHF:
                        set_dictionary((const char*)dictionary_candles_usdchf, sizeof(dictionary_candles_usdchf));
                        break;
                    case USE_DICTIONARY_USDJPY:
                        set_dictionary((const char*)dictionary_candles_usdjpy, sizeof(dictionary_candles_usdjpy));
                        break;
                    case USE_DICTIONARY_USDNOK:
                        set_dictionary((const char*)dictionary_candles_usdnok, sizeof(dictionary_candles_usdnok));
                        break;
                    case USE_DICTIONARY_USDPLN:
                        set_dictionary((const char*)dictionary_candles_usdpln, sizeof(dictionary_candles_usdpln));
                        break;
                    }
#                   else

                    set_dictionary((const char*)dictionary_candles, sizeof(dictionary_candles));

#                   endif
                    break;
                case PRICE_OHLCV:
                    set_dictionary((const char*)dictionary_candles_with_volumes, sizeof(dictionary_candles_with_volumes));
                    break;
                }
            }
            std::vector<std::string> element;
            bf::parse_path(path, element);
            name_ = element.back();
        }

        /** \brief Установить отступ данных от дня загрузки
         * Отсутп позволяет загрузить используемую область данных заранее
         * Это позволит получать значения цены в пределах области без загрузки подфайлов
         * При этом цесли цена выйдет за пределы области, то область сместится, произойдет подзагрузка
         * недостающих данных
         * \param indent_day_dn Отступ от даты загрузки в днях к началу исторических данных
         * \param indent_day_up Отступ от даты загрузки в днях к концу исторических данных
         */
        void set_indent(const int &indent_day_dn, const int &indent_day_up) {
            QuotesHistory::indent_day_up = indent_day_up;
            QuotesHistory::indent_day_dn = indent_day_dn;
        }

        ~QuotesHistory() {}

        /** \brief Записать массив свечей
         * \param candles массив свечей
         * \param timestamp дата массива свечей
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int write_candles(
                const std::array<CANDLE_TYPE, MINUTES_IN_DAY>& candles,
                const ztime::timestamp_t &timestamp) {
            const size_t buffer_size = price_type == PRICE_OHLCV ? CANDLE_WITH_VOLUME_BUFFER_SIZE :
                price_type == PRICE_OHLC ? CANDLE_WITHOUT_VOLUME_BUFFER_SIZE : ONLY_ONE_PRICE_BUFFER_SIZE;

            increase_write_buffer_size(buffer_size);
            char *buffer = write_buffer.get();
            int err_convert = convert_candles_to_buffer(candles, buffer, buffer_size);
            int err_write = 0;
            if(is_use_dictionary) err_write = write_compressed_subfile(ztime::get_day(timestamp), buffer, buffer_size);
            else err_write = write_subfile(ztime::get_day(timestamp), buffer, buffer_size);

            /* если ошибки записи нет, перечитаем фрагмент (если он в памяти),
             * который мы только что записали
             */
            if(err_convert == OK) {
                size_t ind_prices = 0;
                while(ind_prices < candles_array_days.size()) {
                    if(ztime::get_day(candles_array_days[ind_prices][0].timestamp) == ztime::get_day(timestamp)) {
                        read_candles(candles_array_days[ind_prices], ztime::get_day(timestamp), ztime::get_first_timestamp_day(timestamp));
                        break;
                    }
                    ind_prices++;
                }
            }

            return err_convert != OK ? err_convert : err_write;
        }

        /** \brief Получить свечу по временной метке
         * \param candle Свеча/бар
         * \param timestamp метка времени начала свечи
         * \param optimization Оптимизация.
         * Для отключения указать WITHOUT_OPTIMIZATION.
         * По умолчанию включена оптимизация последовательного считывания минут OPTIMIZATION_SEQUENTIAL_READING
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int get_candle(
                CANDLE_TYPE &candle,
                const ztime::timestamp_t &timestamp,
                const int &optimization = OPTIMIZATION_SEQUENTIAL_READING) {
            // сначала проверяем прогноз на запрос на следующую свечу
            if(optimization == WITHOUT_OPTIMIZATION) {
                return find_candle(candle, timestamp);
            } else
            if(optimization == OPTIMIZATION_SEQUENTIAL_READING) {
                if(candles_array_days.size() > 0 &&
                        candles_array_days[ind_forecast_day][ind_forecast_minute].timestamp == timestamp) {
                    candle = candles_array_days[ind_forecast_day][ind_forecast_minute];
                    // прогноз оправдался, делаем следующий прогноз
                    make_next_candles_forecast();
                    return candle.close != 0.0 ? OK : DATA_NOT_AVAILABLE;
                } else {
                    // прогноз не был успешен, делаем поиск котировки
                    int minute_day = ztime::get_minute_day(timestamp);
                    const ztime::timestamp_t first_timestamp_day = ztime::get_first_timestamp_day(timestamp);
                    if(set_start_candles_forecast(first_timestamp_day, minute_day)) {
                        candle = candles_array_days[ind_forecast_day][ind_forecast_minute];
                        return candle.close != 0.0 ? OK : DATA_NOT_AVAILABLE;
                    }
                    // поиск не дал результатов, грузим котировки
                    read_candles_data(first_timestamp_day, indent_day_dn, indent_day_up);
                    if(set_start_candles_forecast(first_timestamp_day, minute_day)) {
                        candle = candles_array_days[ind_forecast_day][ind_forecast_minute];
                        return candle.close != 0.0 ? OK : DATA_NOT_AVAILABLE;
                    }
                    return STRANGE_PROGRAM_BEHAVIOR;
                }
            }
            return INVALID_PARAMETER;
        }

        /** \brief Получить цену (OPEN, HIGH, LOW, CLOSE) по указанной метке времени
         * \param price цена на указанной временной метке
         * \param timestamp временная метка
         * \param price_type тип цены (на выбор: PRICE_CLOSE, PRICE_OPEN, PRICE_LOW, PRICE_HIGH)
         * \param optimization оптимизация чтения данных
         * Для отключения указать WITHOUT_OPTIMIZATION.
         * По умолчанию включена оптимизация последовательного считывания минут OPTIMIZATION_SEQUENTIAL_READING
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int get_price(
                double& price,
                const ztime::timestamp_t &timestamp,
                const int &price_type = PRICE_CLOSE,
                const int &optimization = OPTIMIZATION_SEQUENTIAL_READING) {
            CANDLE_TYPE candle;
            int err = get_candle(candle, timestamp, optimization);
            if(err != OK) {price = 0.0; return OK;}
            price = price_type == PRICE_CLOSE ?
                candle.close : price_type == PRICE_OPEN ?
                candle.open : price_type == PRICE_LOW ?
                candle.low : price_type == PRICE_HIGH ?
                candle.high : candle.close;
            return OK;
        }

        /** \brief Проверить метку времени
         * \warning Данный метод проверяет только наличие дня, а не бара внутри дня
         * \param timestamp метка времени
         * \return вернет true, если метка времени существует
         */
        bool check_timestamp(const ztime::timestamp_t timestamp) {
            return check_subfile(ztime::get_day(timestamp));
        }

        /** \brief Удалить данные за день по метке времени
         *
         * \param timestamp метка времени
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int delete_day(const ztime::timestamp_t timestamp) {

            int err = delete_subfile(ztime::get_day(timestamp));
            return err;
        }

        /** \brief Проверить бинарный опцион
         * Данный метод проверяет состояние бинарного опциона и может иметь три состояния (удачный прогноз, нейтральный и неудачный).
         * При поиске цены входа в опцион данный метод в первую очередь проверит последнюю полученную цену (если используется оптимизация),
         * которая была получена через метод get_candle. Если метка времени последней полученной цены не совпадает с требуемой,
         * то метод начнет поиск цены в хранилище.
         * \warning Будьте аккуратны! Данный метод может создать эффект "подглядывания в будущее"
         * \param state состояние бинарного опциона (удачная сделка WIN = 1, убыточная LOSS = -1 и нейтральная NEUTRAL = 0)
         * \param contract_type тип контракта (доступно BUY = 1 и SELL = -1)
         * \param duration_sec длительность опциона в секундах
         * \param timestamp метка времени начала опциона
         * \param price_type цена входа в опцион (цена закрытия PRICE_CLOSE или открытия PRICE_OPEN свечи)
         * \param optimization оптимизация чтения данных
         * Для отключения указать WITHOUT_OPTIMIZATION.
         * По умолчанию включена оптимизация последовательного считывания минут OPTIMIZATION_SEQUENTIAL_READING
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int check_binary_option(
                int& state,
                const int &contract_type,
                const int &duration_sec,
                const ztime::timestamp_t &timestamp,
                const int &price_type = PRICE_CLOSE,
                const int &optimization = OPTIMIZATION_SEQUENTIAL_READING) {
            if(contract_type != BUY && contract_type != SELL) return INVALID_PARAMETER;
            if(price_type != PRICE_CLOSE && price_type != PRICE_OPEN) return INVALID_PARAMETER;
            const ztime::timestamp_t timestamp_stop = timestamp + duration_sec;
            double price_start, price_stop;
            CANDLE_TYPE candle_start, candle_stop;

            // сначала проверяем прогноз на текущую свечу
            if(optimization == OPTIMIZATION_SEQUENTIAL_READING) {
                if(ind_forecast_minute > 0 && candles_array_days[ind_forecast_day][ind_forecast_minute - 1].timestamp == timestamp) {
                    price_start = price_type == PRICE_CLOSE ? candles_array_days[ind_forecast_day][ind_forecast_minute - 1].close : candles_array_days[ind_forecast_day][ind_forecast_minute - 1].open;
                    if(price_start == 0.0) return DATA_NOT_AVAILABLE;
                } else
                if(ind_forecast_day > 0 && ind_forecast_minute == 0 && candles_array_days[ind_forecast_day - 1][MINUTES_IN_DAY - 1].timestamp == timestamp) {
                    price_start = price_type == PRICE_CLOSE ? candles_array_days[ind_forecast_day - 1][MINUTES_IN_DAY - 1].close : candles_array_days[ind_forecast_day - 1][MINUTES_IN_DAY - 1].open;
                    if(price_start == 0.0) return DATA_NOT_AVAILABLE;
                } else {
                    // придется искать цену
                    int err_candle_start = find_candle(candle_start, timestamp);
                    if(err_candle_start != OK) return DATA_NOT_AVAILABLE;
                    price_start = price_type == PRICE_CLOSE ? candle_start.close : candle_start.open;
                    if(price_start == 0.0) return DATA_NOT_AVAILABLE;
                }
            } else {
                // придется искать цену
                int err_candle_start = find_candle(candle_start, timestamp);
                if(err_candle_start != OK) return DATA_NOT_AVAILABLE;
                price_start = price_type == PRICE_CLOSE ? candle_start.close : candle_start.open;
                if(price_start == 0.0) return DATA_NOT_AVAILABLE;
            }
            int err_candle_stop = find_candle(candle_stop, timestamp_stop);
            if(err_candle_stop != OK) return err_candle_stop;
            price_stop = price_type == PRICE_CLOSE ? candle_stop.close : candle_stop.open;
            if(price_stop == 0.0) return DATA_NOT_AVAILABLE;
            state = price_start != price_stop ? (contract_type == BUY ? (price_start < price_stop ? WIN : LOSS) : (price_start > price_stop ? WIN : LOSS)) : NEUTRAL;
            return OK;
        }

        /** \brief Проверить бинарный опцион с защитой от "подглядывания в будущее".
         * Данный метод проверяет состояние бинарного опциона и может иметь три состояния (удачный прогноз, нейтральный и неудачный).
         * При поиске цены входа в опцион данный метод в первую очередь проверит последнюю полученную цену (если используется оптимизация),
         * которая была получена через метод get_candle. Если метка времени последней полученной цены не совпадает с требуемой,
         * то метод начнет поиск цены в хранилище.
         * \warning Будьте аккуратны! Данный метод может создать эффект "подглядывания в будущее", если неправильно указать last_timestamp!
         * \param state Состояние бинарного опциона (удачная сделка WIN = 1, убыточная LOSS = -1 и нейтральная NEUTRAL = 0).
         * \param contract_type Тип контракта (доступно BUY = 1 и SELL = -1).
         * \param duration_sec Длительность опциона в секундах.
         * \param open_timestamp Метка времени начала опциона.
         * \param last_timestamp Последняя допустимая метка времени.
         * \param price_type Цена входа в опцион (цена закрытия PRICE_CLOSE или открытия PRICE_OPEN свечи).
         * \param optimization Оптимизация чтения данных.
         * Для отключения указать WITHOUT_OPTIMIZATION.
         * По умолчанию включена оптимизация последовательного считывания минут OPTIMIZATION_SEQUENTIAL_READING
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp.
         */
        int check_protected_binary_option(
                int& state,
                const int &contract_type,
                const int &duration_sec,
                const ztime::timestamp_t &open_timestamp,
                const ztime::timestamp_t &last_timestamp,
                const int &price_type = PRICE_CLOSE,
                const int &optimization = OPTIMIZATION_SEQUENTIAL_READING) {
            if((open_timestamp + duration_sec)>= last_timestamp) return DATA_NOT_AVAILABLE;
            return check_binary_option(state, contract_type, duration_sec, open_timestamp, price_type, optimization);
        }

        /** \brief Проверить ордер (метод пока не доделан!)
         * Данный метод вычисляет профит открытого ордера
         * При поиске цены входа в сделку данный метод в первую очередь проверит последнюю полученную цену (если используется оптимизация),
         * которая была получена через метод get_candle. Если метка времени последней полученной цены не совпадает с требуемой,
         * то метод начнет поиск цены в хранилище.
         * \warning Будьте аккуратны! Данный метод может создать эффект "подглядывания в будущее"
         * \param profit Разница между ценами (представлена в единицах котировок цены)
         * \param contract_type тип контракта (доступно BUY = 1 и SELL = -1)
         * \param spread Спред (представлен в единицах котировок цены)
         * \param take_profit Тэйк профит (представлен в единицах котировок цены)
         * \param stop_loss Стоп лосс (представлен в единицах котировок цены)
         * \param timestamp метка времени начала ордера
         * \param timestamp_end метка времени конца ордера (в данную переменную запишется результат)
         * \param price_type цена входа в опцион (цена закрытия PRICE_CLOSE или открытия PRICE_OPEN свечи)
         * \param optimization оптимизация чтения данных
         * Для отключения указать WITHOUT_OPTIMIZATION.
         * По умолчанию включена оптимизация последовательного считывания минут OPTIMIZATION_SEQUENTIAL_READING
         * \param is_ignore_skipping если true, метод будет игнорировать пропуски цен
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int check_oreder(
                double &profit,
                const int &contract_type,
                const double &spread,
                const double &take_profit,
                const double &stop_loss,
                const ztime::timestamp_t &timestamp,
                ztime::timestamp_t &timestamp_end,
                const int &price_type = PRICE_CLOSE,
                const int &optimization = OPTIMIZATION_SEQUENTIAL_READING,
                const bool &is_ignore_skipping = false) {
            profit = 0.0;
            if(price_type != PRICE_CLOSE && price_type != PRICE_OPEN) return INVALID_PARAMETER;
            double price_start, price_stop;
            CANDLE_TYPE candle_start, candle_stop;

            // сначала проверяем прогноз на текущую свечу
            if(optimization == OPTIMIZATION_SEQUENTIAL_READING) {
                if(ind_forecast_minute > 0 && candles_array_days[ind_forecast_day][ind_forecast_minute - 1].timestamp == timestamp) {
                    price_start = price_type == PRICE_CLOSE ? candles_array_days[ind_forecast_day][ind_forecast_minute - 1].close : candles_array_days[ind_forecast_day][ind_forecast_minute - 1].open;
                    if(price_start == 0.0) return DATA_NOT_AVAILABLE;
                } else
                if(ind_forecast_day > 0 && ind_forecast_minute == 0 && candles_array_days[ind_forecast_day - 1][MINUTES_IN_DAY - 1].timestamp == timestamp) {
                    price_start = price_type == PRICE_CLOSE ? candles_array_days[ind_forecast_day - 1][MINUTES_IN_DAY - 1].close : candles_array_days[ind_forecast_day - 1][MINUTES_IN_DAY - 1].open;
                    if(price_start == 0.0) return DATA_NOT_AVAILABLE;
                } else {
                    // придется искать цену
                    int err_candle_start = find_candle(candle_start, timestamp);
                    if(err_candle_start != OK) return DATA_NOT_AVAILABLE;
                    price_start = price_type == PRICE_CLOSE ? candle_start.close : candle_start.open;
                }
            } else {
                // придется искать цену
                int err_candle_start = find_candle(candle_start, timestamp);
                if(err_candle_start != OK) return DATA_NOT_AVAILABLE;
                price_start = price_type == PRICE_CLOSE ? candle_start.close : candle_start.open;
                if(price_start == 0.0) return DATA_NOT_AVAILABLE;
            }

            if(contract_type == BUY) {
                profit = -spread;
                double tl = price_start + spread + take_profit;
                double bl = price_start + spread - stop_loss;
                ztime::timestamp_t t = timestamp;
                while(true) {
                    CANDLE_TYPE candle;
                    int err = get_candle(candle, t);
                    if(err != OK) {
                        if(is_ignore_skipping) {
                            t += ztime::SECONDS_IN_MINUTE;
                            continue;
                        } else return DATA_NOT_AVAILABLE;
                    }
                    if( candle.high == 0.0 ||
                        candle.low == 0.0) {
                        if(is_ignore_skipping) {
                            t += ztime::SECONDS_IN_MINUTE;
                            continue;
                        } else return DATA_NOT_AVAILABLE;
                    }
                    if(candle.low <= bl) {
                        profit = -stop_loss;
                        timestamp_end = t;
                        break;
                    } else
                    if(candle.high >= tl) {
                        profit = take_profit;
                        timestamp_end = t;
                        break;
                    }
                    t += ztime::SECONDS_IN_MINUTE;
                }
            } else
            if(contract_type == SELL) {
                profit = -spread;
                double tl = price_start - spread + stop_loss;
                double bl = price_start - spread - take_profit;
                ztime::timestamp_t t = timestamp;
                while(true) {
                    CANDLE_TYPE candle;
                    int err = get_candle(candle, t);
                    if(err != OK) {
                        if(is_ignore_skipping) {
                            t += ztime::SECONDS_IN_MINUTE;
                            continue;
                        } else return DATA_NOT_AVAILABLE;
                    }
                    if( candle.high == 0.0 ||
                        candle.low == 0.0) {
                        if(is_ignore_skipping) {
                            t += ztime::SECONDS_IN_MINUTE;
                            continue;
                        } else return DATA_NOT_AVAILABLE;
                    }
                    if(candle.high >= tl) {
                        profit = -stop_loss;
                        timestamp_end = t;
                        break;
                    } else
                    if(candle.low <= bl) {
                        profit = take_profit;
                        timestamp_end = t;
                        break;
                    }
                    t += ztime::SECONDS_IN_MINUTE;
                }
            } else return DATA_NOT_AVAILABLE;
            return OK;
        }

        /** \brief Получить имя валютной пары
         * \return имя валютной пары
         */
        inline std::string get_name() {
            return name_;
        }

        /** \brief Получить директорию файлов
         * \return директория файлов
         */
        inline std::string get_path() {
            return path_;
        }

        /** \brief Узнать максимальную и минимальную метку времени
         * \param min_timestamp метка времени в начале дня начала исторических данных
         * \param max_timestamp метка времени в начале дня конца исторических данных
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int get_min_max_day_timestamp(ztime::timestamp_t &min_timestamp, ztime::timestamp_t &max_timestamp) const {
            key_t key_min, key_max;
            int err = get_min_max_key(key_min, key_max);
            if(err == OK) {
                min_timestamp = ztime::SECONDS_IN_DAY*key_min;
                max_timestamp = ztime::SECONDS_IN_DAY*key_max;
            }
            return err;
        }

        /** \brief Получить список дней начиная с заданной метки времени
         * Данный метод позволяет получить список дней, котировки которых можно использовать
         * Список будет содержать метки времени начала дня
         * В список попадает также и день, метка врмени которого указана для поиска (если котировки данного дня существуют)
         * \warning начало дня метки времени start_timestamp входит в список list_timestamp!
         * \param list_timestamp список меток времени начала дня для всех дней начиная с указанной метки времени
         * \param start_timestamp метка времени начала поиска дней для списка
         * \param num_days максимальное количество дней в списке
         * \param is_day_off_filter если true, идет пропуск выходных дней
         * \param is_go_back_in_time если true, от метки времени идет поиск в глубь истории
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int get_day_timestamp_list(
                std::vector<ztime::timestamp_t>& list_timestamp,
                const ztime::timestamp_t &start_timestamp,
                const int &num_days,
                const bool &is_day_off_filter = true,
                const bool &is_go_back_in_time = true) {
            std::vector<key_t> list_subfile;
            int err = OK;
            if(is_day_off_filter) {
                err = get_subfile_list(
                    ztime::get_day(start_timestamp),
                    list_subfile,
                    num_days,
                    check_day_off,
                    is_go_back_in_time);
            } else {
                err = get_subfile_list(
                    ztime::get_day(start_timestamp),
                    list_subfile,
                    num_days,
                    NULL,
                    is_go_back_in_time);
            }
            if(err != OK) return err;
            list_timestamp.clear();
            for(size_t i = 0; i < list_subfile.size(); ++i) {
                list_timestamp.push_back(list_subfile[i] * ztime::SECONDS_IN_DAY);
            }
            return OK;
        }

        /** \brief Торговать
         * \warning метка времени start_timestamp (весь день) входит в диапазон торговли!
         * \param start_timestamp метка времени начала торговли
         * \param stop_timestamp метка времени конца торговли
         * \param step_timestamp шаг метки времени (для минутного таймфрейма ztime::SECONDS_IN_MINUTE)
         * \param f лямбла-функция для обработки торговли
         * \param is_day_off_filter если true, используется фильтр торговли в выходные дни
         * \param is_go_back_in_time проход поиска дней торговли совершается вглубь истории, если true
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int trade(
                const ztime::timestamp_t &start_timestamp,
                const ztime::timestamp_t &stop_timestamp,
                const int &step_timestamp,
                std::function<void (
                    const CANDLE_TYPE &candle,
                    const int err)> f) {
            for(ztime::timestamp_t t = start_timestamp; t <= stop_timestamp; t += step_timestamp) {
                CANDLE_TYPE candle;
                int err = get_candle(candle, t);
                f(candle, err);
            }
            return OK;
        }

        /** \brief Торговать
         * \warning метка времени start_timestamp (весь день) входит в диапазон торговли!
         * \param start_timestamp метка времени поиска начала торговли
         * (начиная с дня метки времени будет запущена торговля)
         * \param step_timestamp шаг метки времени (для минутного таймфрейма ztime::SECONDS_IN_MINUTE)
         * \param num_days количество дней для торговли
         * \param f лямбла-функция для обработки торговли
         * \param is_day_off_filter если true, используется фильтр торговли в выходные дни
         * \param is_go_back_in_time проход поиска дней торговли совершается вглубь истории, если true
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int trade(
                const ztime::timestamp_t &start_timestamp,
                const int &step_timestamp,
                const int &num_days,
                std::function<void (
                    const CANDLE_TYPE &candle,
                    const int day,
                    const int err)> f,
                const bool &is_day_off_filter = true,
                const bool &is_go_back_in_time = true) {
            std::vector<ztime::timestamp_t> list_timestamp;
            int err = get_day_timestamp_list(
                start_timestamp,
                list_timestamp,
                num_days,
                is_day_off_filter,
                is_go_back_in_time);
            if(err != OK || list_timestamp.size() != num_days);
            for(int i = 0; i < num_days; ++i) {
                ztime::timestamp_t stop_timestamp = list_timestamp[i] + ztime::SECONDS_IN_DAY;
                for(ztime::timestamp_t t = list_timestamp[i]; t < stop_timestamp; t += step_timestamp) {
                    CANDLE_TYPE candle;
                    int err = get_candle(candle, t);
                    f(candle, i, err);
                } // for t
            } // for i
            return OK;
        }

        /** \brief Получить количество знаков после запятой
         * \param decimal_places количество знаков после запятой (или множитель, если is_factor = true)
         * \param is_factor При установке данного флага функция возвращает множитель
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int get_decimal_places(int &decimal_places, const bool &is_factor = false) {
            if(decimal_places_ != 0) {
                decimal_places = decimal_places_;
                return OK;
            }
            ztime::timestamp_t min_timestamp = 0;
            ztime::timestamp_t max_timestamp = 0;
            int err = get_min_max_day_timestamp(min_timestamp, max_timestamp);
            max_timestamp += ztime::SECONDS_IN_DAY;
            if(err != OK) return err;

            const size_t MAX_CANDLES = 10000;
            ztime::timestamp_t step = (max_timestamp - min_timestamp) / MAX_CANDLES;
            if(step < ztime::SECONDS_IN_MINUTE) step = ztime::SECONDS_IN_MINUTE;

            std::vector<CANDLE_TYPE> candles;
            CANDLE_TYPE old_candle;
            for(ztime::timestamp_t t = min_timestamp; t < max_timestamp; t+= step) {
                CANDLE_TYPE candle;
                int err = get_candle(candle, t);
                if( err == OK &&
                    (candle.close != 0 || candle.open != 0 || candle.low != 0 || candle.high != 0) &&
                    (old_candle.close != candle.close || old_candle.open != candle.open || old_candle.high != candle.high || old_candle.low != candle.low)) {
                    candles.push_back(candle);
                    old_candle = candle;
                }
            }
            if(candles.size() == 0) return DATA_NOT_AVAILABLE;
            decimal_places = xquotes_common::get_decimal_places(candles, is_factor);
            decimal_places_ = decimal_places;
            return OK;
        }
    };

    /** \brief Класс для удобного использования исторических данных нескольких валютных пар
     */
    template <class CANDLE_TYPE = Candle>
    class MultipleQuotesHistory {
    private:

        std::vector<std::shared_ptr<QuotesHistory<CANDLE_TYPE>>> symbols; /**< Вектор с историческими данными цен */
        ztime::timestamp_t min_timestamp = 0;                                              /**< Временная метка начала исторических данных по всем валютным парам */
        ztime::timestamp_t max_timestamp = std::numeric_limits<ztime::timestamp_t>::max(); /**< Временная метка конца исторических данных по всем валютным парам */
        bool is_init = false;

        /** \brief Проверка выходного дня
         */
        static bool check_day_off(const key_t &key) {
            return ztime::is_day_off_for_day(key);
        }

    public:
        MultipleQuotesHistory() {};

        /** \brief Инициализировать класс
         * \param paths директории с файлами исторических данных
         * \param price_type тип цены (на выбор: PRICE_CLOSE, PRICE_OHLC, PRICE_OHLCV)
         * \param option настройки хранилища котировок (использовать сжатие  - USE_COMPRESSION, иначе DO_NOT_USE_COMPRESSION)
         */
        MultipleQuotesHistory(
                const std::vector<std::string> &paths,
                const int &price_type = PRICE_OHLC,
                const int &option = USE_COMPRESSION) {
            for(size_t i = 0; i < paths.size(); ++i) {
                symbols.push_back(std::make_shared<QuotesHistory<CANDLE_TYPE>>(paths[i], price_type, option));
            }
            for(size_t i = 0; i < paths.size(); ++i) {
                ztime::timestamp_t symbol_min_timestamp = 0, symbol_max_timestamp = 0;
                if(symbols[i]->get_min_max_day_timestamp(symbol_min_timestamp, symbol_max_timestamp) == OK) {
                    if(symbol_max_timestamp < max_timestamp) max_timestamp = symbol_max_timestamp;
                    if(symbol_min_timestamp > min_timestamp) min_timestamp = symbol_min_timestamp;
                }
            }
            is_init = true;
        }

        ~MultipleQuotesHistory() {
            //for(size_t i = 0; i < symbols.size(); ++i) {
            //    delete symbols[i];
            //    symbols[i] = NULL;
            //}
            //symbols.clear();
        }

        /** \brief Получить указатель на класс исторических данных указанного символа
         * \param symbol_ind Индекс символа
         * \return Вернет указатель на класс исторических данны или NULL в случае ошибки
         */
        QuotesHistory<CANDLE_TYPE>* get_quotes_history(const size_t &symbol_ind) {
            if(symbol_ind < symbols.size()) return symbols[symbol_ind].get();
            return NULL;
        }

        /** \brief Установить отступ данных от дня загрузки
         * Отсутп позволяет загрузить используемую область данных заранее
         * Это позволит получать значения цены в пределах области без загрузки подфайлов
         * При этом цесли цена выйдет за пределы области, то область сместится, произойдет подзагрузка
         * недостающих данных
         * \param indent_day_dn Отступ от даты загрузки в днях к началу исторических данных
         * \param indent_day_up Отступ от даты загрузки в днях к концу исторических данных
         * \param symbol_ind Номер символа
         * \return Вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int set_indent(const int &indent_day_dn, const int &indent_day_up, const int &symbol_ind) {
            if(symbol_ind >= (int)symbols.size()) return INVALID_PARAMETER;
            symbols[symbol_ind]->set_indent(indent_day_dn, indent_day_up);
            return OK;
        }

        /** \brief Установить отступ данных от дня загрузки
         * Отсутп позволяет загрузить используемую область данных заранее
         * Это позволит получать значения цены в пределах области без загрузки подфайлов
         * При этом цесли цена выйдет за пределы области, то область сместится, произойдет подзагрузка
         * недостающих данных
         * \param indent_day_dn Отступ от даты загрузки в днях к началу исторических данных
         * \param indent_day_up Отступ от даты загрузки в днях к концу исторических данных
         */
        void set_indent(const int &indent_day_dn, const int &indent_day_up) {
            for(size_t i = 0; i < symbols.size(); ++i) {
                symbols[i]->set_indent(indent_day_dn, indent_day_up);
            }
        }

        /** \brief Получить число символов в классе исторических данных
         * \return число символов, валютных пар, индексов и пр. вместе взятых
         */
        inline int get_num_symbols() {
            return symbols.size();
        }

        /** \brief Узнать максимальную и минимальную метку времени
         * \param min_timestamp Метка времени в начале дня начала исторических данных
         * \param max_timestamp Метка времени в начале дня конца исторических данных
         * \return Вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        inline int get_min_max_day_timestamp(
                ztime::timestamp_t &min_timestamp,
                ztime::timestamp_t &max_timestamp) {
            min_timestamp = MultipleQuotesHistory::min_timestamp;
            max_timestamp = MultipleQuotesHistory::max_timestamp;
            if(!is_init) return NO_INIT;
            return OK;
        }

        /** \brief Узнать максимальную и минимальную метку времени конкретного символа
         * \param min_timestamp метка времени в начале дня начала исторических данных
         * \param max_timestamp метка времени в начале дня конца исторических данных
         * \param symbol_ind номер символа
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        inline int get_min_max_day_timestamp(
                ztime::timestamp_t &min_timestamp,
                ztime::timestamp_t &max_timestamp,
                const int &symbol_ind) {
            if(symbol_ind >= (int)symbols.size()) return INVALID_PARAMETER;
            return symbols[symbol_ind]->get_min_max_start_day_timestamp(min_timestamp, max_timestamp);
        }

        /** \brief Получить свечу по временной метке
         * \param candle Свеча/бар
         * \param timestamp временная метка начала свечи
         * \param symbol_ind номер символа
         * \param optimization оптимизация.
         * Для отключения указать WITHOUT_OPTIMIZATION.
         * По умолчанию включена оптимизация последовательного считывания минут OPTIMIZATION_SEQUENTIAL_READING
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int get_candle(
                CANDLE_TYPE &candle,
                const ztime::timestamp_t &timestamp,
                const int &symbol_ind,
                const int &optimization = OPTIMIZATION_SEQUENTIAL_READING) {
            if(symbol_ind >= (int)symbols.size()) return INVALID_PARAMETER;
            return symbols[symbol_ind]->get_candle(candle, timestamp, optimization);
        }

        /** \brief Получить цену (OPEN, HIGH, LOW, CLOSE) по указанной метке времени
         * \param price цена на указанной временной метке
         * \param timestamp временная метка
         * \param symbol_ind номер символа
         * \param price_type тип цены (на выбор: PRICE_CLOSE, PRICE_OPEN, PRICE_LOW, PRICE_HIGH)
         * \param optimization оптимизация
         * Для отключения указать WITHOUT_OPTIMIZATION.
         * По умолчанию включена оптимизация последовательного считывания минут OPTIMIZATION_SEQUENTIAL_READING
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int get_price(
                double& price,
                const ztime::timestamp_t &timestamp,
                const int &symbol_ind,
                const int &price_type = PRICE_CLOSE,
                const int &optimization = OPTIMIZATION_SEQUENTIAL_READING) {
            if(symbol_ind >= (int)symbols.size()) return INVALID_PARAMETER;
            return symbols[symbol_ind]->get_price(price, timestamp, price_type, optimization);
        }

        /** \brief Получить свечи всех символов по метке времени
         * \param symbols_candle Свечи/бар всех символов
         * \param timestamp временная метка начала свечи
         * \param optimization оптимизация.
         * Для отключения указать WITHOUT_OPTIMIZATION.
         * По умолчанию включена оптимизация последовательного считывания минут OPTIMIZATION_SEQUENTIAL_READING
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int get_symbols_candle(
                std::vector<CANDLE_TYPE> &symbols_candle,
                const ztime::timestamp_t &timestamp,
                const int &optimization = OPTIMIZATION_SEQUENTIAL_READING) {
            symbols_candle.resize(symbols.size());
            int err = OK;
            for(size_t i = 0; i < symbols.size(); ++i) {
                int symbol_err = symbols[i]->get_candle(symbols_candle[i], timestamp, optimization);
                if(symbol_err != OK) {
                    err = symbol_err;
                }
            }
            return err;
        }

        /** \brief Получить цены всех символов по метке времени
         * \param symbols_price цены всех символов
         * \param timestamp метка времени начала свечи
         * \param price_type тип цены (на выбор: PRICE_CLOSE, PRICE_OPEN, PRICE_LOW, PRICE_HIGH)
         * \param optimization оптимизация.
         * Для отключения указать WITHOUT_OPTIMIZATION.
         * По умолчанию включена оптимизация последовательного считывания минут OPTIMIZATION_SEQUENTIAL_READING
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int get_symbols_price(
                std::vector<double> &symbols_price,
                const ztime::timestamp_t &timestamp,
                const int &price_type = PRICE_CLOSE,
                const int &optimization = OPTIMIZATION_SEQUENTIAL_READING) {
            symbols_price.resize(symbols.size());
            int err = OK;
            for(size_t i = 0; i < symbols.size(); ++i) {
                int symbol_err = symbols[i]->get_price(symbols_price[i], timestamp, price_type, optimization);
                if(symbol_err != OK) {
                    err = symbol_err;
                }
            }
            return err;
        }

        /** \brief Проверить бинарный опцион конкретного символа
         * Данный метод проверяет состояние бинарного опциона и может иметь три состояния (удачный прогноз, нейтральный и неудачный).
         * При поиске цены входа в опцион данный метод в первую очередь проверит последнюю полученную цену,
         * которая была получена через метод get_candle. Если метка времени последней полученной цены не совпадает с требуемой,
         * то метод начнет поиск цены в хранилище.
         * \param state состояние бинарного опциона (удачная сделка WIN = 1, убыточная LOSS = -1 и нейтральная NEUTRAL = 0)
         * \param contract_type тип контракта (доступно BUY = 1 и SELL = -1)
         * \param duration_sec длительность опциона в секундах
         * \param timestamp временная метка начала опциона
         * \param price_type цена входа в опцион (цена закрытия PRICE_CLOSE или открытия PRICE_OPEN свечи)
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int check_binary_option(
                int& state,
                const int &contract_type,
                const int &duration_sec,
                const ztime::timestamp_t &timestamp,
                const int &symbol_ind,
                const int &price_type = PRICE_CLOSE,
                const int &optimization = OPTIMIZATION_SEQUENTIAL_READING) {
            if(symbol_ind >= (int)symbols.size()) return INVALID_PARAMETER;
            return symbols[symbol_ind]->check_binary_option(state, contract_type, duration_sec, timestamp, price_type, optimization);
        }

        /** \brief Получить имя валютной пары по индексу символа
         * \param symbol_ind индекс символа
         * \return имя символа, если символ существует, иначе пустая строка
         */
        inline std::string get_name(const int &symbol_ind) {
            if(symbol_ind >= (int)symbols.size()) return "";
            return symbols[symbol_ind]->get_name();
        }

        /** \brief Получить директорию файла по индексу символа
         * \param symbol_ind индекс символа
         * \return директория файла символа, если символ существует, иначе пустая строка
         */
        inline std::string get_path(const int &symbol_ind) {
            if(symbol_ind >= (int)symbols.size()) return "";
            return symbols[symbol_ind]->get_path();
        }

        /** \brief Получить список меток времени начала дня начиная с заданной метки вреемни поиска
         * \warning метка времени start_timestamp (начало дня) входит в список list_timestamp!
         * \param list_timestamp список меток времени начала дня для всех дней, начиная с указанной метки времени
         * \param start_timestamp метка времени начала поиска дней для списка
         * \param is_symbol
         * \param num_days максимальное количество дней в списке
         * \param is_day_off_filter если true, идет пропуск выходных дней
         * \param is_go_back_in_time если true, от метки времени идет поиск в глубь истории
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int get_day_timestamp_list(
                std::vector<ztime::timestamp_t>& list_timestamp,
                const ztime::timestamp_t &start_timestamp,
                const std::vector<bool> &is_symbol,
                const int &num_days,
                const bool &is_day_off_filter = true,
                const bool &is_go_back_in_time = true) {
            if(symbols.size() == 0) return DATA_NOT_AVAILABLE;
            if(is_symbol.size() != symbols.size()) return INVALID_PARAMETER;
            std::vector<key_t> list_subfile;
            std::vector<key_t> old_list_subfile;
            int err = OK;
            auto filter_day = is_day_off_filter ? check_day_off : NULL;
            for(size_t i = 0; i < symbols.size(); ++i) {
                if(!is_symbol[i]) continue;
                err = symbols[i]->get_subfile_list(
                    ztime::get_day(start_timestamp),
                    list_subfile,
                    num_days,
                    filter_day,
                    is_go_back_in_time);
                // если спики стали отличаться или есть ошибка
                if((i > 0 && old_list_subfile != list_subfile) || err != OK) {
                    if(old_list_subfile != list_subfile) err = STRANGE_PROGRAM_BEHAVIOR;
                    break;
                }
                old_list_subfile = list_subfile;
            }
            if(err != OK) return err;
            list_timestamp.clear();
            for(size_t i = 0; i < list_subfile.size(); ++i) {
                list_timestamp.push_back(list_subfile[i] * ztime::SECONDS_IN_DAY);
            }
            return OK;
        }

        /** \brief Получить количество знаков после запятой символа
         * \param decimal_places количество знаков после запятой
         * \param symbol_ind индекс символа
         * \param is_factor При установке данного флага функция возвращает множитель
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int get_decimal_places(
                int &decimal_places,
                const int &symbol_ind,
                const bool &is_factor = false) {
            if(symbol_ind >= (int)symbols.size()) return INVALID_PARAMETER;
            return symbols[symbol_ind]->get_decimal_places(decimal_places, is_factor);
            return OK;
        }

        /** \brief Торговать
         * \warning метка времени start_timestamp (весь день) входит в диапазон торговли!
         * \param start_timestamp метка времени поиска начала торговли
         * (начиная с дня метки времени будет запущена торговля)
         * \param step_timestamp шаг метки времени (для минутного таймфрейма ztime::SECONDS_IN_MINUTE)
         * \param num_days количество дней для торговли
         * \param is_symbol список флагов, разрешающих торговлю на конкретном символе
         * \param f лямбда-функция для обработки торговли
         * \param is_day_off_filter если true, используется фильтр торговли в выходные дни
         * \param is_go_back_in_time проход поиска дней торговли совершается вглубь истории, если true
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int trade(
                const ztime::timestamp_t &start_timestamp,
                const int &step_timestamp,
                const int &num_days,
                const std::vector<bool> &is_symbol,
                std::function<void(
                    const CANDLE_TYPE &candle,
                    const int day,
                    const int symbol_ind,
                    const int err)> f,
                const bool &is_day_off_filter = true,
                const bool &is_go_back_in_time = true) {
            if(is_symbol.size() != symbols.size()) return INVALID_PARAMETER;
            std::vector<ztime::timestamp_t> list_timestamp;
            int err = get_day_timestamp_list(
                list_timestamp,
                start_timestamp,
                is_symbol,
                num_days,
                is_day_off_filter,
                is_go_back_in_time);

            if(err != OK || (int)list_timestamp.size() != num_days) return DATA_NOT_AVAILABLE;
            const int num_symbols = symbols.size();
            for(int i = 0; i < num_days; ++i) {
                ztime::timestamp_t stop_timestamp = list_timestamp[i] + ztime::SECONDS_IN_DAY;
                for(ztime::timestamp_t t = list_timestamp[i]; t < stop_timestamp; t += step_timestamp) {
                    for(int s = 0; s < num_symbols; ++s) {
                        if(!is_symbol[s]) continue;
                        CANDLE_TYPE candle;
                        int err = get_candle(candle, t, s);
                        f(candle, i, s, err);
                    } // for s
                } // for t
            } // for i
            return OK;
        }

#       ifndef XQUOTES_DO_NOT_USE_THREAD
        /** \brief Торговать в несколько потоков
         * \warning метка времени start_timestamp (весь день) входит в диапазон торговли!
         * \param start_timestamp метка времени поиска начала торговли
         * (начиная с дня метки времени будет запущена торговля)
         * \param step_timestamp шаг метки времени (для минутного таймфрейма ztime::SECONDS_IN_MINUTE)
         * \param num_days количество дней для торговли
         * \param is_symbol список флагов, разрешающих торговлю на конкретном символе
         * \param f лямбда-функция для обработки торговли
         * \param is_day_off_filter если true, используется фильтр торговли в выходные дни
         * \param is_go_back_in_time проход поиска дней торговли совершается вглубь истории, если true
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int trade_multiple_threads(
                const ztime::timestamp_t &start_timestamp,
                const int &step_timestamp,
                const int &num_days,
                const std::vector<bool> &is_symbol,
                std::function<void(
                    const CANDLE_TYPE &candle,
                    const int day,
                    const int symbol_ind,
                    const int thread_ind,
                    const int err)> f,
                const bool &is_day_off_filter = true,
                const bool &is_go_back_in_time = true) {
            if(is_symbol.size() != symbols.size()) return INVALID_PARAMETER;
            std::vector<ztime::timestamp_t> list_timestamp;
            int err = get_day_timestamp_list(
                list_timestamp,
                start_timestamp,
                is_symbol,
                num_days,
                is_day_off_filter,
                is_go_back_in_time);

            if(err != OK || (int)list_timestamp.size() != num_days) return DATA_NOT_AVAILABLE;

            // создаем новый список символов с учетом фильтра символов
            const int num_symbols = symbols.size();
            std::vector<int> list_symbol_ind;
            for(int s = 0; s < num_symbols; ++s) {
                if(!is_symbol[s]) continue;
                list_symbol_ind.push_back(s);
            }
            const int num_list_symbol = list_symbol_ind.size();

            // получаем максимальное количество потоков процессора и создаем массив потоков
            const int num_hardware_thread = std::thread::hardware_concurrency();
            const int num_thread = std::min((int)list_symbol_ind.size(),num_hardware_thread);
            std::vector<std::thread> list_thread;
            list_thread.resize(num_thread);

            for(int thread_ind = 0; thread_ind < num_thread; ++thread_ind) {
                list_thread[thread_ind] = std::thread([&, thread_ind, num_thread]() {
                    for(int i = 0; i < num_days; ++i) {
                        ztime::timestamp_t stop_timestamp = list_timestamp[i] + ztime::SECONDS_IN_DAY;
                        for(ztime::timestamp_t t = list_timestamp[i]; t < stop_timestamp; t += step_timestamp) {
                            for(int s = thread_ind; s < num_list_symbol; s += num_thread) {
                                int symbol_ind = list_symbol_ind[s];
                                CANDLE_TYPE candle;
                                int err = get_candle(candle, t, symbol_ind);
                                f(candle, i, symbol_ind, thread_ind, err);
                            } // for s
                        } // for t
                    } // for i
                }); // std::thread
            } // for thread_ind
            for(size_t i = 0; i < list_thread.size(); ++i) {
                list_thread[i].join();
            }
            return OK;
        }
#       endif

    };
}

#endif // XQUOTES_HISTORY_HPP_INCLUDED
