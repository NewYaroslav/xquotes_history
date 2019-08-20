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
// подключаем словари для сжатия файлов
#include "xquotes_dictionary_candles.hpp"
#include "xquotes_dictionary_candles_with_volumes.hpp"
#include "xquotes_dictionary_only_one_price.hpp"

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
        typedef std::array<CANDLE_TYPE, MINUTES_IN_DAY> candles_array_t;
        //char *dictionary_buffer = NULL;    /**< Указатель на буффер для хранения словаря */
        bool is_use_dictionary = false;
        int price_type = PRICE_CLOSE;
        std::string path_;
        std::string name_;

        std::vector<candles_array_t> candles_array_days; /**< Котировки в виде минутных свечей, отсортирован по дням */

        /** \brief Получить метку времени начала дня массива цен по индексу элемента
         * Данный метод нужен для внутреннего использования
         * \param indx индекс элемента массива цен
         * \return метка времени начала дня
         */
        inline xtime::timestamp_t get_prices_timestamp(const int indx) {
            return candles_array_days[indx][0].timestamp;
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
        typename std::vector<candles_array_t>::iterator find_candles_array(const xtime::timestamp_t timestamp) {
            if(candles_array_days.size() == 0) return candles_array_days.end();
            auto prices_it = std::lower_bound(
                candles_array_days.begin(),
                candles_array_days.end(),
                timestamp,
                [](const candles_array_t &lhs, const xtime::timestamp_t &timestamp) {
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

        int indx_forecast_day = 0;      /**< Прогноз индекса (дня) в массиве candles_array_days */
        int indx_forecast_minute = 0;   /**< Прогноз индекса в массиве свечей candles_array_days[indx_forecast_day] */

        /** \brief Сделать следующий прогноз для идексов массива свечей
         * Данный метод нужен для внутреннего использования
         */
        inline void make_next_candles_forecast() {
            indx_forecast_minute++;
            if(indx_forecast_minute >= MINUTES_IN_DAY) {
                indx_forecast_minute = 0;
                if(indx_forecast_day >= (candles_array_days.size() - 1)) return;
                indx_forecast_day++;
            }
        }

        /** \brief Установить начальное положение идексов массива цен
         * Данный метод нужен для внутреннего использования
         * \param timestamp временная метка (должна быть всегда в начале дня!)
         * \param minute_day минута дня
         * \return вернет true если цена была найдена
         */
        inline bool set_start_candles_forecast(const xtime::timestamp_t timestamp, const int minute_day) {
            auto found_candles_array = find_candles_array(timestamp);
            if(found_candles_array == candles_array_days.end()) return false;
            indx_forecast_day = found_candles_array - candles_array_days.begin();
            indx_forecast_minute = minute_day;
            return true;
        }

        /** \brief Заполнить метки времени массива свечей
         * Данный метод нужен для внутреннего использования
         */
        void fill_timestamp(std::array<CANDLE_TYPE, MINUTES_IN_DAY>& candles, const xtime::timestamp_t timestamp) {
            candles[0].timestamp = timestamp;
            for(int i = 1; i < MINUTES_IN_DAY; ++i) {
                candles[i].timestamp = candles[i - 1].timestamp + xtime::SECONDS_IN_MINUTE;
            }
        }

        /** \brief Конвертировать массив свечей в буфер
         * Данный метод нужен для внутреннего использования
         */
        int convert_candles_to_buffer(
                const std::array<CANDLE_TYPE, MINUTES_IN_DAY>& candles,
                char* buffer,
                const unsigned long buffer_size) {
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
                    int indx = i * BUFFER_SAMPLE_SIZE;
                    ((price_t*)buffer)[indx + 0] = convert_to_uint(candles[i].open);
                    ((price_t*)buffer)[indx + 1] = convert_to_uint(candles[i].high);
                    ((price_t*)buffer)[indx + 2] = convert_to_uint(candles[i].low);
                    ((price_t*)buffer)[indx + 3] = convert_to_uint(candles[i].close);
                }
            } else
            if(buffer_size == CANDLE_WITH_VOLUME_BUFFER_SIZE) {
                const int BUFFER_SAMPLE_SIZE = 5;
                for(int i = 0; i < MINUTES_IN_DAY; ++i) {
                    int indx = i * BUFFER_SAMPLE_SIZE;
                    ((price_t*)buffer)[indx + 0] = convert_to_uint(candles[i].open);
                    ((price_t*)buffer)[indx + 1] = convert_to_uint(candles[i].high);
                    ((price_t*)buffer)[indx + 2] = convert_to_uint(candles[i].low);
                    ((price_t*)buffer)[indx + 3] = convert_to_uint(candles[i].close);
                    ((price_t*)buffer)[indx + 4] = convert_to_uint(candles[i].volume);
                }
            } else {
                std::cout << "buffer_size " << buffer_size << std::endl;
                return INVALID_ARRAY_LENGH;
            }
            return OK;
        }

        /** \brief Конвертировать буфер в массив свечей
         * Данный метод нужен для внутреннего использования
         */
        int convert_buffer_to_candles(std::array<CANDLE_TYPE, MINUTES_IN_DAY>& candles, const char* buffer, const unsigned long buffer_size) {
            if(buffer_size == ONLY_ONE_PRICE_BUFFER_SIZE) {
                for(int i = 0; i < MINUTES_IN_DAY; ++i) {
                    candles[i].close = convert_to_double(((price_t*)buffer)[i]);
                }
            } else
            if(buffer_size == CANDLE_WITHOUT_VOLUME_BUFFER_SIZE) {
                const int BUFFER_SAMPLE_SIZE = 4;
                for(int i = 0; i < MINUTES_IN_DAY; ++i) {
                    int indx = i * BUFFER_SAMPLE_SIZE;
                    candles[i].open = convert_to_double(((price_t*)buffer)[indx + 0]);
                    candles[i].high = convert_to_double(((price_t*)buffer)[indx + 1]);
                    candles[i].low = convert_to_double(((price_t*)buffer)[indx + 2]);
                    candles[i].close = convert_to_double(((price_t*)buffer)[indx + 3]);
                }
            } else
            if(buffer_size == CANDLE_WITH_VOLUME_BUFFER_SIZE) {
                const int BUFFER_SAMPLE_SIZE = 5;
                for(int i = 0; i < MINUTES_IN_DAY; ++i) {
                    int indx = i * BUFFER_SAMPLE_SIZE;
                    candles[i].open = convert_to_double(((price_t*)buffer)[indx + 0]);
                    candles[i].high = convert_to_double(((price_t*)buffer)[indx + 1]);
                    candles[i].low = convert_to_double(((price_t*)buffer)[indx + 2]);
                    candles[i].close = convert_to_double(((price_t*)buffer)[indx + 3]);
                    candles[i].volume = convert_to_double(((price_t*)buffer)[indx + 4]);
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
        int read_candles(std::array<CANDLE_TYPE, MINUTES_IN_DAY>& candles, const key_t key, const xtime::timestamp_t timestamp) {
            int err = 0;
            char *buffer = NULL;
            unsigned long buffer_size = 0;
            fill_timestamp(candles, timestamp);
            if(is_use_dictionary) err = read_compressed_subfile(key, buffer, buffer_size);
            else err = read_subfile(key, buffer, buffer_size);
            if(err != OK) {
                delete buffer;
                return err;
            }
            err = convert_buffer_to_candles(candles, buffer, buffer_size);
            delete buffer;
            return err;
        }

        /** \brief Прочитать данные
         * \warning Данный метод нужен для внутреннего использования
         * \param timestamp временная метка требуемого дня котировоки (это обязательно начало дня!)
         * \param indent_dn отступ от временная метки в днях на уменьшение времени
         * \param indent_up отступ от временная метки в днях на увеличение времени
         */
        void read_candles_data(const xtime::timestamp_t timestamp, const int indent_dn, const int indent_up) {
            int num_days = indent_dn + indent_up + 1;
            xtime::timestamp_t start_indx_timestamp = timestamp - indent_dn * xtime::SECONDS_IN_DAY;
            int start_indx_day = xtime::get_day(start_indx_timestamp);
            if(candles_array_days.size() == 0) {
                // в данном случае просто загружаем данные
                candles_array_days.resize(num_days);
                int indx_day = start_indx_day;
                xtime::timestamp_t indx_timestamp = start_indx_timestamp;
                for(int i = 0; i < num_days; ++i, ++indx_day, indx_timestamp += xtime::SECONDS_IN_DAY) {
                    read_candles(candles_array_days[i], indx_day, indx_timestamp);
                }
            } else {
                // сначала составим список уже загруженных данных
                std::vector<xtime::timestamp_t> check_timestamp;
                for(size_t j = 0; j < candles_array_days.size(); ++j) {
                    bool is_rewrite = true;
                    int indx_day = start_indx_day;
                    xtime::timestamp_t indx_timestamp = start_indx_timestamp;
                    for(int i = 0; i < num_days; ++i, ++indx_day, indx_timestamp += xtime::SECONDS_IN_DAY) {
                        if(std::binary_search(check_timestamp.begin(), check_timestamp.end(), indx_timestamp)) continue;
                        if(get_prices_timestamp(j) == indx_timestamp) {
                            auto check_timestamp_it = std::upper_bound(check_timestamp.begin(),check_timestamp.end(), indx_timestamp);
                            check_timestamp.insert(check_timestamp_it, indx_timestamp);
                        }
                    }
                }
                // удаляем лишние данные
                size_t indx_prices = 0;
                while(indx_prices < candles_array_days.size()) {
                    if(!std::binary_search(check_timestamp.begin(), check_timestamp.end(), candles_array_days[indx_prices][0].timestamp)) {
                        candles_array_days.erase(candles_array_days.begin() + indx_prices);
                        continue;
                    }
                    indx_prices++;
                }
                // далее загрузим недостающие данные
                int indx_day = start_indx_day;
                xtime::timestamp_t indx_timestamp = start_indx_timestamp;
                for(int i = 0; i < num_days; ++i, ++indx_day, indx_timestamp += xtime::SECONDS_IN_DAY) {
                    if(std::binary_search(check_timestamp.begin(), check_timestamp.end(), indx_timestamp)) continue;
                    candles_array_days.resize(candles_array_days.size() + 1);
                    read_candles(candles_array_days[candles_array_days.size() - 1], indx_day, indx_timestamp);
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
        int find_candle(T &candle, const xtime::timestamp_t timestamp) {
            int minute_day = xtime::get_minute_day(timestamp);
            const xtime::timestamp_t timestamp_start_day = xtime::get_start_day(timestamp);
            auto found_candles_array = find_candles_array(timestamp);
            if(found_candles_array == candles_array_days.end()) {
                read_candles_data(timestamp_start_day, indent_day_dn, indent_day_up);
                found_candles_array = find_candles_array(timestamp);
                if(found_candles_array == candles_array_days.end()) return STRANGE_PROGRAM_BEHAVIOR;
            }
            int indx_day = found_candles_array - candles_array_days.begin();
            candle = candles_array_days[indx_day][minute_day];
            return candle.close != 0.0 ? OK : DATA_NOT_AVAILABLE;
        }

        void update_file_notes(int new_price_type) {
            const unsigned int NOTES_MASK = 0x0F;
            const unsigned int COMPRESSION_BIT = 0x10;
            if(get_num_subfiles() == 0) {
                note_t notes = is_use_dictionary ? COMPRESSION_BIT : 0x00;
                notes |= new_price_type & NOTES_MASK;
                set_file_note(notes);
                QuotesHistory::price_type = new_price_type;
            } else {
                note_t notes = get_file_note();
                QuotesHistory::price_type = notes & NOTES_MASK;
                is_use_dictionary = notes & COMPRESSION_BIT ? true : false;
            }
        }

        public:

        QuotesHistory() : Storage() {};

        /** \brief Инициализировать класс
         * \param path директория с файлами исторических данных
         * \param dictionary_file файл словаря (если указано "", то считываются несжатые файлы)
         */
        QuotesHistory(const std::string path, const int price_type, const std::string dictionary_file = "") :
                Storage(path, dictionary_file), path_(path) {
            std::vector<std::string> element;
            bf::parse_path(path, element);
            name_ = element.back();
            if(dictionary_file.size() > 0) is_use_dictionary = true;
            update_file_notes(price_type);
        }

        /** \brief Инициализировать класс
         * \param path путь к файлу с данными
         * \param dictionary_buffer указатель на буфер словаря
         * \param dictionary_buffer_size размер буфера словаря
         */
        QuotesHistory(const std::string path, const int price_type, const char *dictionary_buffer, const size_t dictionary_buffer_size) :
                Storage(path, dictionary_buffer, dictionary_buffer_size), path_(path) {
            std::vector<std::string> element;
            bf::parse_path(path, element);
            name_ = element.back();
            is_use_dictionary = true;
            update_file_notes(price_type);
        }

        /** \brief Инициализировать класс
         * \param path путь к файлу с данными
         * \param price_type тип цены (на выбор: PRICE_CLOSE, PRICE_OHLC, PRICE_OHLCV)
         * \param option настройки хранилища котировок (использовать сжатие  - USE_COMPRESSION, иначе DO_NOT_USE_COMPRESSION)
         */
        QuotesHistory(const std::string path, const int price_type, const int option) :
                Storage(path), path_(path) {
            int dictionary_buffer_size = 0;
            if(option == USE_COMPRESSION) is_use_dictionary = true;
            update_file_notes(price_type);
            if(is_use_dictionary) {
                switch (price_type){
                case PRICE_CLOSE:
                case PRICE_OPEN:
                    set_dictionary((const char*)dictionary_only_one_price, sizeof(dictionary_only_one_price));
                    break;
                case PRICE_OHLC:
                    set_dictionary((const char*)dictionary_candles, sizeof(dictionary_candles));
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

        ~QuotesHistory() {}

        /** \brief Записать массив свечей
         * \param candles массив свечей
         * \param timestamp дата массива свечей
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int write_candles(
                const std::array<CANDLE_TYPE, MINUTES_IN_DAY>& candles,
                const xtime::timestamp_t timestamp) {
            const size_t buffer_size = price_type == PRICE_OHLCV ? CANDLE_WITH_VOLUME_BUFFER_SIZE :
                price_type == PRICE_OHLC ? CANDLE_WITHOUT_VOLUME_BUFFER_SIZE : ONLY_ONE_PRICE_BUFFER_SIZE;
            char *buffer = new char[buffer_size];
            int err_convert = convert_candles_to_buffer(candles, buffer, buffer_size);
            int err_write = 0;
            if(is_use_dictionary) err_write = write_compressed_subfile(xtime::get_day(timestamp), buffer, buffer_size);
            else err_write = write_subfile(xtime::get_day(timestamp), buffer, buffer_size);
            delete buffer;
            return err_convert != OK ? err_convert : err_write;
        }

        /** \brief Получить свечу по временной метке
         * \param candle Свеча/бар
         * \param timestamp временная метка начала свечи
         * \param optimization оптимизация.
         * Для отключения указать WITHOUT_OPTIMIZATION.
         * По умолчанию включена оптимизация последовательного считывания минут OPTIMIZATION_SEQUENTIAL_READING
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int get_candle(CANDLE_TYPE &candle, const xtime::timestamp_t timestamp, const int optimization = OPTIMIZATION_SEQUENTIAL_READING) {
            // сначала проверяем прогноз на запрос на следующую свечу
            if(optimization == WITHOUT_OPTIMIZATION) {
                return find_candle(candle, timestamp);
            } else
            if(optimization == OPTIMIZATION_SEQUENTIAL_READING) {
                if(candles_array_days.size() > 0 && candles_array_days[indx_forecast_day][indx_forecast_minute].timestamp == timestamp) {
                    candle = candles_array_days[indx_forecast_day][indx_forecast_minute];
                    // прогноз оправдался, делаем следующий прогноз
                    make_next_candles_forecast();
                    return candle.close != 0.0 ? OK : DATA_NOT_AVAILABLE;
                } else {
                    // прогноз не был успешен, делаем поиск котировки
                    int minute_day = xtime::get_minute_day(timestamp);
                    const xtime::timestamp_t timestamp_start_day = xtime::get_start_day(timestamp);
                    if(set_start_candles_forecast(timestamp_start_day, minute_day)) {
                        candle = candles_array_days[indx_forecast_day][indx_forecast_minute];
                        return candle.close != 0.0 ? OK : DATA_NOT_AVAILABLE;
                    }
                    // поиск не дал результатов, грузим котировки
                    read_candles_data(timestamp_start_day, indent_day_dn, indent_day_up);
                    if(set_start_candles_forecast(timestamp_start_day, minute_day)) {
                        candle = candles_array_days[indx_forecast_day][indx_forecast_minute];
                        return candle.close != 0.0 ? OK : DATA_NOT_AVAILABLE;
                    }
                    return STRANGE_PROGRAM_BEHAVIOR;
                }
            }
            return INVALID_PARAMETER;
        }

        /** \brief Проверить бинарный опцион
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
                const int contract_type,
                const int duration_sec,
                const xtime::timestamp_t timestamp,
                const int price_type = PRICE_CLOSE,
                const int optimization = OPTIMIZATION_SEQUENTIAL_READING) {
            if(contract_type != BUY && contract_type != SELL) return INVALID_PARAMETER;
            if(price_type != PRICE_CLOSE && price_type != PRICE_OPEN) return INVALID_PARAMETER;
            const xtime::timestamp_t timestamp_stop = timestamp + duration_sec;
            double price_start, price_stop;
            CANDLE_TYPE candle_start, candle_stop;

            // сначала проверяем прогноз на текущую свечу
            if(optimization == OPTIMIZATION_SEQUENTIAL_READING) {
                if(indx_forecast_minute > 0 && candles_array_days[indx_forecast_day][indx_forecast_minute - 1].timestamp == timestamp) {
                    price_start = price_type == PRICE_CLOSE ? candles_array_days[indx_forecast_day][indx_forecast_minute - 1].close : candles_array_days[indx_forecast_day][indx_forecast_minute - 1].open;
                    if(price_start == 0.0) return DATA_NOT_AVAILABLE;
                } else
                if(indx_forecast_day > 0 && indx_forecast_minute == 0 && candles_array_days[indx_forecast_day - 1][MINUTES_IN_DAY - 1].timestamp == timestamp) {
                    price_start = price_type == PRICE_CLOSE ? candles_array_days[indx_forecast_day - 1][MINUTES_IN_DAY - 1].close : candles_array_days[indx_forecast_day - 1][MINUTES_IN_DAY - 1].open;
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
            }
            int err_candle_stop = find_candle(candle_stop, timestamp_stop);
            if(err_candle_stop != OK) return err_candle_stop;
            price_stop = price_type == PRICE_CLOSE ? candle_stop.close : candle_stop.open;
            state = price_start != price_stop ? (contract_type == BUY ? (price_start < price_stop ? WIN : LOSS) : (price_start > price_stop ? WIN : LOSS)) : NEUTRAL;
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
        int get_min_max_start_day_timestamp(xtime::timestamp_t &min_timestamp, xtime::timestamp_t &max_timestamp) {
            key_t key_min, key_max;
            int err = get_min_max_key(key_min, key_max);
            if(err == OK) {
                min_timestamp = xtime::SECONDS_IN_DAY*key_min;
                max_timestamp = xtime::SECONDS_IN_DAY*key_max;
            }
            return err;
        }

        int trade(
                const xtime::timestamp_t start_timestamp,
                const xtime::timestamp_t stop_timestamp,
                const int step_timestamp,
                std::function<void (
                    const QuotesHistory<CANDLE_TYPE> &hist,
                    const CANDLE_TYPE &candle,
                    const int err)> f) {
            for(xtime::timestamp_t t = start_timestamp; t <= stop_timestamp; t += step_timestamp) {
                CANDLE_TYPE candle;
                int err = get_candle(candle, t);
                //f(candle, err);
                f(*this, candle, err);
            }
        }
    };

    /** \brief Класс для удобного использования исторических данных нескольких валютных пар
     */
    template <class CANDLE_TYPE = Candle>
    class MultipleQuotesHistory {
    private:

        std::vector<QuotesHistory<CANDLE_TYPE>*> symbols; /**< Вектор с историческими данными цен */
        xtime::timestamp_t min_timestamp = 0;                                              /**< Временная метка начала исторических данных по всем валютным парам */
        xtime::timestamp_t max_timestamp = std::numeric_limits<xtime::timestamp_t>::max(); /**< Временная метка конца исторических данных по всем валютным парам */
        bool is_init = false;

    public:
        MultipleQuotesHistory() {};

        /** \brief Инициализировать класс
         * \param paths директории с файлами исторических данных
         * \param dictionary_file файл словаря (если указано "", то считываются несжатые файлы)
         */
        MultipleQuotesHistory(std::vector<std::string> paths, const int price_type, const int option = USE_COMPRESSION) {
            for(size_t i = 0; i < paths.size(); ++i) {
                symbols.push_back(new QuotesHistory<CANDLE_TYPE>(paths[i], price_type, option));
            }
            for(size_t i = 0; i < paths.size(); ++i) {
                xtime::timestamp_t symbol_min_timestamp = 0, symbol_max_timestamp = 0;
                if(symbols[i]->get_min_max_start_day_timestamp(symbol_min_timestamp, symbol_max_timestamp) == OK) {
                    if(symbol_max_timestamp < max_timestamp) max_timestamp = symbol_max_timestamp;
                    if(symbol_min_timestamp > min_timestamp) min_timestamp = symbol_max_timestamp;
                }
            }
            is_init = true;
        }

        /** \brief Получить число символов в классе исторических данных
         * \return число символов, валютных пар, индексов и пр. вместе взятых
         */
        int get_num_symbols() {
            return symbols.size();
        }

        /** \brief Узнать максимальную и минимальную метку времени
         * \param min_timestamp метка времени в начале дня начала исторических данных
         * \param max_timestamp метка времени в начале дня конца исторических данных
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        inline int get_min_max_start_day_timestamp(xtime::timestamp_t &min_timestamp, xtime::timestamp_t &max_timestamp) {
            MultipleQuotesHistory::min_timestamp = min_timestamp;
            MultipleQuotesHistory::max_timestamp = max_timestamp;
            if(!is_init) return NO_INIT;
            return OK;
        }

        /** \brief Узнать максимальную и минимальную метку времени конкретного символа
         * \param min_timestamp метка времени в начале дня начала исторических данных
         * \param max_timestamp метка времени в начале дня конца исторических данных
         * \param symbol_indx номер символа
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        inline int get_min_max_start_day_timestamp(xtime::timestamp_t &min_timestamp, xtime::timestamp_t &max_timestamp, int symbol_indx) {
            if(symbol_indx >= (int)symbols.size()) return INVALID_PARAMETER;
            return symbols[symbol_indx]->get_min_max_start_day_timestamp(min_timestamp, max_timestamp);
        }

        /** \brief Получить свечу по временной метке
         * \param candle Свеча/бар
         * \param timestamp временная метка начала свечи
         * \param symbol_indx номер символа
         * \param optimization оптимизация.
         * Для отключения указать WITHOUT_OPTIMIZATION.
         * По умолчанию включена оптимизация последовательного считывания минут OPTIMIZATION_SEQUENTIAL_READING
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        template <typename T>
        int get_candle(CANDLE_TYPE &candle, const xtime::timestamp_t timestamp, const int symbol_indx, const int optimization = OPTIMIZATION_SEQUENTIAL_READING) {
            if(symbol_indx >= (int)symbols.size()) return INVALID_PARAMETER;
            return symbols[symbol_indx]->get_candle(candle, timestamp, optimization);
        }

        /** \brief Получить свечи всех символов по временной метке
         * \param symbols_candle Свечи/бар всех валютных пар
         * \param timestamp временная метка начала свечи
         * \param optimization оптимизация.
         * Для отключения указать WITHOUT_OPTIMIZATION.
         * По умолчанию включена оптимизация последовательного считывания минут OPTIMIZATION_SEQUENTIAL_READING
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int get_symbols_candle(std::vector<CANDLE_TYPE> &symbols_candle, const xtime::timestamp_t timestamp, const int optimization = OPTIMIZATION_SEQUENTIAL_READING) {
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
                const int contract_type,
                const int duration_sec,
                const xtime::timestamp_t timestamp,
                const int symbol_indx,
                const int price_type = PRICE_CLOSE,
                const int optimization = OPTIMIZATION_SEQUENTIAL_READING) {
            if(symbol_indx >= (int)symbols.size()) return INVALID_PARAMETER;
            return symbols[symbol_indx]->check_binary_option(state, contract_type, duration_sec, timestamp, price_type, optimization);
        }

        /** \brief Получить имя валютной пары по индексу символа
         * \param symbol_indx индекс символа
         * \return имя символа, если символ существует, иначе пустая строка
         */
        inline std::string get_name(int symbol_indx) {
            if(symbol_indx >= (int)symbols.size()) return "";
            return symbols[symbol_indx]->get_name();
        }

        /** \brief Получить директорию файла по индексу символа
         * \param symbol_indx индекс символа
         * \return директория файла символа, если символ существует, иначе пустая строка
         */
        inline std::string get_path(int symbol_indx) {
            if(symbol_indx >= (int)symbols.size()) return "";
            return symbols[symbol_indx]->get_path();
        }
    };
}

#endif // XQUOTES_HISTORY_HPP_INCLUDED
