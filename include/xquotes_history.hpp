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
#ifndef XQUOTES_HISTORY_HPP_INCLUDED
#define XQUOTES_HISTORY_HPP_INCLUDED

#include "xquotes_storage.hpp"
#include <array>

namespace xquotes_history {
    using namespace xquotes_common;
    using namespace xquotes_storage;

    /** \brief Класс для удобного использования исторических данных
     * Данный класс имеет оптимизированный для поминутного чтения данных метод - get_candle
     * Оптимизация метода get_candle заключается в том, что поиск следующей цены начинается только в случае,
     * если метка времени следующей цены не совпала с запрашиваемой меткой времени
     * Метод find_candle является по сути аналогом get_candle, но он всегда ищет цену
     */
    class QuotesHistory : public Storage {
    private:
        typedef std::array<Candle, MINUTES_IN_DAY> candles_array_t;

        bool is_use_dictionary = false;
        std::string path_;
        std::string name_;
        std::string file_extension_;

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
        std::vector<candles_array_t>::iterator find_candles_array(const xtime::timestamp_t timestamp) {
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
            return false;
        }

        /** \brief Заполнить метки времени массива свечей
         * Данный метод нужен для внутреннего использования
         */
        void fill_timestamp(std::array<Candle, MINUTES_IN_DAY>& candles, const xtime::timestamp_t timestamp) {
            candles[0].timestamp = timestamp;
            for(int i = 1; i < MINUTES_IN_DAY; ++i) {
                candles[i].timestamp = candles[i - 1].timestamp + xtime::SECONDS_IN_MINUTE;
            }
        }

        /** \brief Конвертировать буфер в массив свечей
         * Данный метод нужен для внутреннего использования
         */
        int convert_buffer_to_candles(std::array<Candle, MINUTES_IN_DAY>& candles, const char* buffer, const unsigned long buffer_size) {
            const unsigned long ONLY_CLOSING_PRICE_BUFFER_SIZE = MINUTES_IN_DAY * sizeof(price_t);
            const unsigned long CANDLE_WITHOUT_VOLUME_BUFFER_SIZE = MINUTES_IN_DAY * 4 * sizeof(price_t);
            const unsigned long CANDLE_WITH_VOLUME = MINUTES_IN_DAY * 5 * sizeof(price_t);
            if(buffer_size == ONLY_CLOSING_PRICE_BUFFER_SIZE) {
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
            }
            if(buffer_size == CANDLE_WITH_VOLUME) {
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
         * Данный метод нужен для внутреннего использования
         * \param candles массив свечей за день
         * \param key ключ, это день с начала unix времени
         * \param timestamp метка времени (должна быть всегда в начале дня!)
         * \return состояние ошибки
         */
        int read_candles(std::array<Candle, MINUTES_IN_DAY>& candles, const key_t key, const xtime::timestamp_t timestamp) {
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
//------------------------------------------------------------------------------
        public:

        /** \brief Инициализировать класс
         * \param path директория с файлами исторических данных
         * \param dictionary_file файл словаря (если указано "", то считываются несжатые файлы)
         */
        QuotesHistory(const std::string path, const std::string dictionary_file = "") :
                Storage(path, dictionary_file), path_(path) {
            std::vector<std::string> element;
            bf::parse_path(path, element);
            name_ = element.back();
            if(dictionary_file != "") {
                file_extension_ = ".zstd";
            } else {
                file_extension_ = ".hex";
            }
        }

        int write_candles(
                std::array<Candle, MINUTES_IN_DAY>& candles,
                const xtime::timestamp_t timestamp,
                const int price_type = ALL_PRICE) {

        }

        /** \brief Получить свечу по временной метке
         * \param candle Свеча/бар
         * \param timestamp временная метка начала свечи
         * \param optimization оптимизация.
         * Для отключения указать WITHOUT_OPTIMIZATION.
         * По умолчанию включена оптимизация последовательного считывания минут OPTIMIZATION_SEQUENTIAL_READING
         * \return вернет 0 в случае успеха
         */
        template <typename T>
        int get_candle(T &candle, const xtime::timestamp_t timestamp, const int optimization = OPTIMIZATION_SEQUENTIAL_READING) {
            // сначала проверяем прогноз на запрос на следующую свечу
            if(optimization == WITHOUT_OPTIMIZATION) {
                return find_candle(candle, timestamp);
            } else
            if(optimization == OPTIMIZATION_SEQUENTIAL_READING)
            if(candles_array_days[indx_forecast_day][indx_forecast_minute].timestamp == timestamp) {
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
         * \return состояние ошибки (0 в случае успеха)
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
            Candle candle_start, candle_stop;

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

        /** \brief Получить расширение файла
         * \return расширение файла
         */
        inline std::string get_file_extension() {
            return file_extension_;
        }

        /** \brief Получить данные массива цен
         * \return Массив цен
         */
        inline std::vector<double> get_array_prices() {
            //return prices;
        }

        /** \brief Получить данные массива временных меток
         * \return Массив временных меток
         */
        inline std::vector<unsigned long long> get_array_times() {
            //return times;
        }

        /** \brief Получить цену закрытия свечи на указанной временной метке
         * \param price цена на указанной временной метке
         * \param timestamp временная метка
         * \return состояние огибки, 0 если все в порядке
         */
        int get_price(double& price, const xtime::timestamp_t timestamp) {

        }

        /** \brief Получить данные цен тиков или цен закрытия свечей
         * Внимание! Для цен закрытия свечей указывается временная метка НАЧАЛА свечи
         * \param prices цены тиков или цены закрытия свечей
         * \param data_size количество данных для записи в prices
         * \param step шаг времени
         * \param timestamp временная метка
         * \return состояние огибки, 0 если все в порядке
         */
        int get_prices(std::vector<double>& prices, int data_size, int step, unsigned long long timestamp) {

        }

        /** \brief Прочитать все данные из директории
         * \param beg_timestamp временная метка начала чтения исторических данных
         * \param end_timestamp временная метка конца чтения исторических данных
         * \return вернет 0 в случае успеха
         */
        int read_all_data(unsigned long long beg_timestamp, unsigned long long end_timestamp) {

        }

        /** \brief Прочитать все данные из директории
         * Данная функция загружает все доступные данные
         * \return вернет 0 в случае успеха
         */
        int read_all_data() {

        }

        /** \brief Получить новую цену
         * Данную функцию можно вызывать после read_all_data чтобы последовательно считывать данные из массива.
         * \param price цена
         * \param timestamp временная метка
         * \param status состояние данных (END_OF_DATA - конец данных, SKIPPING_DATA - пропущен бар или тик, NORMAL_DATA - данные считаны нормально)
         * \param period_data период данных (для минутных свечей 60, для тиков на сайте binary - 1 секунда)
         * \return вернет 0 в случае успеха
         */
        int get_price(double &price, unsigned long long &timestamp, int& status, int period_data = 60){

        }

    };

    /** \brief Класс для удобного использования исторических данных нескольких валютных пар
     */
    class MultipleQuotesHistory {
    private:

        std::vector<QuotesHistory> symbols;                /**< Вектор с историческими данными цен */
        unsigned long long beg_timestamp = 0;                   /**< Временная метка начала исторических данных по всем валютным парам */
        unsigned long long end_timestamp = 0;                   /**< Временная метка конца исторических данных по всем валютным парам */
        bool is_init = false;

    public:

        /** \brief Инициализировать класс
         * \param paths директории с файлами исторических данных
         * \param dictionary_file файл словаря (если указано "", то считываются несжатые файлы)
         */
        MultipleQuotesHistory(std::vector<std::string> paths, std::string dictionary_file = "") {
            for(size_t i = 0; i < paths.size(); ++i) {
                //symbols.push_back(CurrencyHistory(paths[i], dictionary_file));
            }
            //if(BinaryApiEasy::get_beg_end_timestamp_for_paths(paths, ".", beg_timestamp, end_timestamp) == OK) {
            //    if(beg_timestamp != end_timestamp) {
                    is_init = true;
            //    }
            //}
        }

        /** \brief Получить число валют в классе исторических данных
         * \return число валют
         */
        int get_number_currencies() {
            if(is_init){
                //return currencies.size();
            } else return NO_INIT;
        }
//------------------------------------------------------------------------------
        /** \brief Найти первую и последнюю дату файлов
         * \param beg_timestamp первая дата, встречающееся среди файлов
         * \param end_timestamp последняя дата, встречающееся среди файлов
         * \return вернет 0 в случае успеха
         */
        inline int get_beg_end_timestamp(unsigned long long &_beg_timestamp, unsigned long long &_end_timestamp) {
            _beg_timestamp = beg_timestamp;
            _end_timestamp = end_timestamp;
            if(!is_init) {
                return NO_INIT;
            }
            return OK;
        }
//------------------------------------------------------------------------------
        /** \brief Получить данные цен тиков или цен закрытия свечей
         * Внимание! Для цен закрытия свечей указывается временная метка НАЧАЛА свечи
         * \param array_prices массив массивов цен тиков или цен закрытия свечей
         * \param data_size количество данных для записи в массивы массива array_prices
         * \param step шаг времени
         * \param timestamp временная метка
         * \return состояние огибки, 0 если все в порядке
         */
        int get_prices(std::vector<std::vector<double>>& array_prices, int data_size, int step, unsigned long long timestamp) {
            if(!is_init) {
                return NO_INIT;
            }
            array_prices.resize(symbols.size());
            for(size_t i = 0; i < symbols.size(); ++i) {
                int err = symbols[i].get_prices(array_prices[i], data_size, step, timestamp);
                if(err != OK) {
                    return err;
                }
            }
            return OK;
        }

        /** \brief Получить массив цен со всех валютных пар по временной метке
         * \param prices массив цен со всех валютных пар за данную веремнную метку
         * \param timestamp временная метка
         * \return вернет 0 в случае успеха
         */
        int get_price(std::vector<double> &prices, unsigned long long timestamp) {
            if(!is_init) {
                return NO_INIT;
            }
            prices.resize(symbols.size());
            for(size_t i = 0; i < symbols.size(); ++i) {
                int err = symbols[i].get_price(prices[i], timestamp);
                if(err != OK) {
                    return err;
                }
            }
            return OK;
        }

        /** \brief Получить массив цен со всех валютных пар по временной метке
         * \param prices массив цен со всех валютных пар за данную веремнную метку
         * \param timestamp временная метка
         * \return вернет 0 в случае успеха
         */
        int get_price(double &price, unsigned long long timestamp, int index) {
            if(!is_init) return NO_INIT;
            return symbols[index].get_price(price, timestamp);
        }

        /** \brief Проверить бинарный опцион
         * \param state состояние бинарного опциона (уданая сделка WIN = 1, убыточная LOSS = -1 и нейтральная 0)
         * \param contract_type тип контракта (см. ContractType, доступно BUY и SELL)
         * \param duration_sec длительность опциона в секундах
         * \param timestamp временная метка начала опциона
         * \param indx позиция валютной пары в массиве валютных пар (должно совпадать с позицией в массиве paths)
         * \return состояние ошибки (0 в случае успеха)
         */
        int check_binary_option(int& state, int contract_type, int duration_sec, unsigned long long timestamp, int indx) {
                if(!is_init) {
                        return NO_INIT;
                }
                if(indx >= (int)symbols.size()) {
                        return INVALID_PARAMETER;
                }
                return symbols[indx].check_binary_option(state, contract_type, duration_sec, timestamp);
        }

                /** \brief Получить имя валютной пары по индексу
                 * \param index индекс валютной пары
                 * \return имя валютной пары
                 */
                inline std::string get_name(int index)
                {
                        return symbols[index].get_name();
                }

                /** \brief Получить директорию файлов
                 * \param index индекс валютной пары
                 * \return директория файлов
                 */
                inline std::string get_path(int index)
                {
                        return symbols[index].get_path();
                }

                /** \brief Получить расширение файла
                 * \param index индекс валютной пары
                 * \return расширение файла
                 */
                inline std::string get_file_extension(int index)
                {
                        return symbols[index].get_file_extension();
                }

        };

}

#endif // XQUOTES_HISTORY_HPP_INCLUDED
