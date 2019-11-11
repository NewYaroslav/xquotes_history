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

/** \file Файл с шаблоном класса хранилища дневных данных
 * \brief Данный файл содержит шаблон класса DailyDataStorage хранилища данных
 *
 * Класс DailyDataStorage подходит для хранения данных, разбитых по дням. Ключ подфайла является меткой времени начала данных.
 * Класс данных должен иметь:
 * метод empty
 * метод assign
 * метод data
 * метод size
 */
#ifndef XQUOTES_DAILY_DATA_STORAGE_HPP_INCLUDED
#define XQUOTES_DAILY_DATA_STORAGE_HPP_INCLUDED

#include "xquotes_storage.hpp"
#include <memory>
#include <utility>

namespace xquotes_daily_data_storage {
    using namespace xquotes_common;

    /** \brief Шаблон класса для храннения данных, разбитых по дням
     *
     * Шаблон класса подходит для хранения данных, разбитых по дням. Ключ подфайла является меткой времени начала данных.
     * Класс данных должен иметь:
     * метод empty
     * метод assign
     * метод data
     * метод size
     * Пустой конструктор
     */
    template <class T>
	class DailyDataStorage : public xquotes_storage::Storage {
        private:
        T null_day_data;
        std::vector<std::pair<T,xtime::timestamp_t>> array_daily_data;  /**< Данные подфайлов, отсортированые по дням */
        std::string path_daily_data_storage;            /**< Путь к хранилищу данных */
        std::string name_daily_data_storage;            /**< Имя хранилища данных */
        bool is_use_dictionary = false;                 /**< Флаг словаря для сжатия подфайлов */
        std::unique_ptr<char[]> read_day_data_buffer;   /**< Буфер для чтения статистики */
        size_t read_day_data_buffer_size = 0;           /**< Размер буфера для чтения статистики */

        /** \brief Сортировка массива подфайлов
         * \warning Данный метод нужен для внутреннего использования
         */
        void sort_array_daily_data() {
            if(!std::is_sorted(array_daily_data.begin(), array_daily_data.end(),
                [](const std::pair<T,xtime::timestamp_t> &a, const std::pair<T,xtime::timestamp_t> &b) {
                        return a.second < b.second;
                    })) {
                std::sort(array_daily_data.begin(), array_daily_data.end(),
                [](const std::pair<T,xtime::timestamp_t> &a, const std::pair<T,xtime::timestamp_t> &b) {
                    return a.second < b.second;
                });
            }
        }

        /** \brief Найти подфайл за конкретный день
         *
         * Метод ищет подфайл конкретного дня по метке времени.
         * \warning Данный метод нужен для внутреннего использования
         * \param timestamp метка времени (должна быть всегда в начале дня!)
         * \return указатель на подфайл
         */
        typename std::vector<std::pair<T,xtime::timestamp_t>>::iterator find_day_data(const xtime::timestamp_t timestamp) {
            if(array_daily_data.size() == 0) return array_daily_data.end();
            auto array_daily_data_it = std::lower_bound(
                array_daily_data.begin(),
                array_daily_data.end(),
                timestamp,
                [](const std::pair<T, xtime::timestamp_t> &lhs, const xtime::timestamp_t &timestamp) {
                return lhs.second < timestamp;
            });
            if(array_daily_data_it == array_daily_data.end()) {
                return array_daily_data.end();
            } else
            if(array_daily_data_it->second == timestamp) {
                return array_daily_data_it;
            }
            return array_daily_data.end();
        }

        /** \brief Прочитать подфайл за конкретный день
         * \warning Данный метод нужен для внутреннего использования
         * \param day_data Данные за день
         * \param key Ключ подфайла. Это день с начала unix времени
         * \return Код ошибки
         */
        int read_day_data(std::pair<T,xtime::timestamp_t> &day_data, const xquotes_common::key_t key) {
            /* устанавливаем метку времени */
            const xtime::timestamp_t timestamp = xtime::SECONDS_IN_DAY * key;
            day_data.second = timestamp;
            unsigned long buffer_size = 0;
            int err = xquotes_common::OK;
            if(is_use_dictionary) err = read_compressed_subfile(key, read_day_data_buffer, read_day_data_buffer_size, buffer_size);
            else err = read_subfile(key, read_day_data_buffer, read_day_data_buffer_size, buffer_size);
            if(err != OK) return err;
            /* загруждаем данные в класс подфайла */
            const char* day_data_buffer = read_day_data_buffer.get();
            day_data.first.assign(day_data_buffer, (size_t)buffer_size);
            return err;
        }

        /** \brief Прочитать дневные данные
         * \warning Данный метод нужен для внутреннего использования
         * \param timestamp Метка времени начала дня запрашиваемого подфайла
         * \param indent_timestamp_past Отступ от метки времени к прошлому. Размерность: дни
         * \param indent_timestamp_future Отступ от метки времени к будущему. Размерность: дни
         */
        void read_daily_data(
                const xtime::timestamp_t timestamp,
                const uint32_t indent_timestamp_past,
                const uint32_t indent_timestamp_future) {
            /* получаем количество дней для загрузки в массив */
            const uint32_t days = indent_timestamp_past + indent_timestamp_future + 1;
            /* получаем начальную метку времени для загрузки данных в массив и начальный ключ подфайла  */
            const xtime::timestamp_t start_timestamp = timestamp - indent_timestamp_past * xtime::SECONDS_IN_DAY;
            const uint32_t start_day_data_key = xtime::get_day(start_timestamp);
            if(array_daily_data.size() == 0) {
                /* если данных в массиве нет, просто загружаем данные */
                array_daily_data.resize(days);
                uint32_t day_data_key = start_day_data_key;
                for(uint32_t day = 0; day < days; ++day, ++day_data_key) {
                    read_day_data(array_daily_data[day], day_data_key);
                }
            } else {
                /* если данные уже есть в массиве, сначала составим список уже загруженных данных */
                std::vector<xtime::timestamp_t> check_timestamp;
                for(size_t j = 0; j < array_daily_data.size(); ++j) {
                    xtime::timestamp_t timestamp_day = start_timestamp;
                    for(uint32_t day = 0; day < days; ++day, timestamp_day += xtime::SECONDS_IN_DAY) {
                        if(std::binary_search(check_timestamp.begin(), check_timestamp.end(), timestamp_day)) continue;
                        if(array_daily_data[j].second == timestamp_day) {
                            auto check_timestamp_it = std::upper_bound(check_timestamp.begin(),check_timestamp.end(), timestamp_day);
                            check_timestamp.insert(check_timestamp_it, timestamp_day);
                        }
                    } // for i
                } // for j

                /* удаляем лишние данные, которые отсутствуют в списке check_timestamp */
                size_t index_day_data = 0;
                while(index_day_data < array_daily_data.size()) {
                    if(!std::binary_search(check_timestamp.begin(), check_timestamp.end(), array_daily_data[index_day_data].second)) {
                        array_daily_data.erase(array_daily_data.begin() + index_day_data);
                        continue;
                    }
                    ++index_day_data;
                }

                /* далее загрузим недостающие данные */
                uint32_t day_data_key = start_day_data_key;
                xtime::timestamp_t day_data_timestamp = start_timestamp;
                for(uint32_t day = 0; day < days; ++day, ++day_data_key, day_data_timestamp += xtime::SECONDS_IN_DAY) {
                    if(std::binary_search(check_timestamp.begin(), check_timestamp.end(), day_data_timestamp)) continue;
                    array_daily_data.resize(array_daily_data.size() + 1);
                    read_day_data(array_daily_data[array_daily_data.size() - 1], day_data_key);
                }
            }
            /* сортируем массив данных по метке времени */
            sort_array_daily_data();
        }

        uint32_t indent_timestamp_future = 1;   /**< Отступ в будущее от текущей метки времени. Единица измерения: дни */
        uint32_t indent_timestamp_past = 1;     /**< Отступ в прошлое от текущей метки времени. Единица измерения: дни */
        uint32_t index_forecast_array = 0;  /**< Прогноз индекса в массиве array_daily_data */

        /** \brief Сделать следующий прогноз для идексов массива свечей
         * \warning Данный метод нужен для внутреннего использования
         */
        inline void make_next_forecast() {
            if(index_forecast_array >= ((uint32_t)array_daily_data.size() - 1)) return;
            ++index_forecast_array;
        }

        /** \brief Установить начальное положение идекса массива статистики
         * \warning Данный метод нужен для внутреннего использования
         * \param timestamp Метка времени (должна быть всегда в начале дня!)
         * \return Вернет true если цена была найдена
         */
        inline bool set_start_forecast(const xtime::timestamp_t timestamp) {
            auto found_day_data = find_day_data(timestamp);
            if(found_day_data == array_daily_data.end()) return false;
            index_forecast_array = found_day_data - array_daily_data.begin();
            return true;
        }

        /** \brief Найти подфайл по временной метке
         * \warning Данный метод нужен для внутреннего использования
         * \param day_data Данные за указанный день
         * \param timestamp временная метка начала свечи
         * \return Вернет 0 в случае успеха
         */
        int find_day_data(T& day_data, const xtime::timestamp_t timestamp) {
            const xtime::timestamp_t first_timestamp_day = xtime::get_first_timestamp_day(timestamp);
            auto found_day_data = find_day_data(timestamp);
            if(found_day_data == array_daily_data.end()) {
                read_daily_data(first_timestamp_day, indent_timestamp_past, indent_timestamp_future);
                found_day_data = find_day_data(timestamp);
                if(found_day_data == array_daily_data.end()) return STRANGE_PROGRAM_BEHAVIOR;
            }
            const uint32_t index = found_day_data - array_daily_data.begin();
            day_data = array_daily_data[index].first;
            return !day_data.empty() ? OK : DATA_NOT_AVAILABLE;
        }

        /** \brief Получить ссылку на подфайл
         * \warning Данный метод нужен для внутреннего использования
         * \param timestamp временная метка начала свечи
         * \return Вернет ссылку на данные
         */
        T& get_link_day_data(const xtime::timestamp_t timestamp) {
            const xtime::timestamp_t first_timestamp_day = xtime::get_first_timestamp_day(timestamp);
            auto found_day_data = find_day_data(timestamp);
            if(found_day_data == array_daily_data.end()) {
                read_daily_data(first_timestamp_day, indent_timestamp_past, indent_timestamp_future);
                found_day_data = find_day_data(timestamp);
                if(found_day_data == array_daily_data.end()) return null_day_data;
            }
            const uint32_t index = found_day_data - array_daily_data.begin();
            return array_daily_data[index].first;
        }
    public:

        DailyDataStorage() : Storage() {};

        /** \brief Инициализировать хранилище статистики
         * \param path Путь к файлу хранилища
         * \param dictionary_ptr Указатель на словарь
         * \param dictionary_size Размер словаря
         */
        DailyDataStorage(
                const std::string &path,
                const char* dictionary_ptr = NULL,
                const size_t dictionary_size = 0) :
                Storage(path), path_daily_data_storage(path) {
            if(dictionary_ptr != NULL && dictionary_size != 0) {
                set_dictionary(dictionary_ptr, dictionary_size);
                is_use_dictionary = true;
            }
            std::vector<std::string> element;
            bf::parse_path(path, element);
            name_daily_data_storage = element.back();
        };

        /** \brief Получить имя хранилища
         * \return имя валютной пары
         */
        inline std::string get_name() {
            return name_daily_data_storage;
        }

        /** \brief Получить директорию файлов
         * \return директория файлов
         */
        inline std::string get_path() {
            return path_daily_data_storage;
        }

        /** \brief Установить отступ данных от дня загрузки
         *
         * Отсутп позволяет загрузить используемую область данных заранее.
         * Это позволит получать данные в пределах области без повторной загрузки подфайлов.
         * При этом, цесли данные выйдут за пределы области, то область сместится, произойдет подзагрузка
         * недостающих данных.
         * \param indent_timestamp_past Отступ от даты загрузки в днях к началу исторических данных
         * \param indent_timestamp_future Отступ от даты загрузки в днях к концу исторических данных
         */
        void set_indent(
                const uint32_t indent_timestamp_past,
                const uint32_t indent_timestamp_future) {
            DailyDataStorage::indent_timestamp_past = indent_timestamp_past;
            DailyDataStorage::indent_timestamp_future = indent_timestamp_future;
        }

        /** \brief Записать данные за день
         * \param day_data Данные одного дня
         * \param timestamp Метка времени начала дня указанных данных
         * \return Вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int write_day_data(const T& day_data, const xtime::timestamp_t timestamp) {
            const size_t buffer_size = day_data.size();
            int err_write = xquotes_common::OK;
            if(is_use_dictionary) err_write = write_compressed_subfile(xtime::get_day(timestamp), (const char*)day_data.data(), buffer_size);
            else err_write = write_subfile(xtime::get_day(timestamp), (const char*)day_data.data(), buffer_size);
            return err_write;
        }

        /** \brief Получить данные за указанный день
         *
         * \warning Данный метод не защищает от подглядывания в будущее!
         * \param day_data Данные за указанный день
         * \param timestamp Метка времени
         * \param optimization Тип оптимизации доступа к данным
         * \return Вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int get_day_data(
                T &day_data,
                const xtime::timestamp_t timestamp,
                const int optimization = xquotes_common::OPTIMIZATION_SEQUENTIAL_READING) {
            if(optimization == xquotes_common::WITHOUT_OPTIMIZATION) {
                return find_day_data(day_data, timestamp);
            } else
            if(optimization == xquotes_common::OPTIMIZATION_SEQUENTIAL_READING) {
                const xtime::timestamp_t first_timestamp_day = xtime::get_first_timestamp_day(timestamp);
                if( array_daily_data.size() > 0 &&
                    array_daily_data[index_forecast_array].second == first_timestamp_day) {
                    day_data = array_daily_data[index_forecast_array].first;
                    // прогноз оправдался, делаем следующий прогноз
                    make_next_forecast();
                    return !day_data.empty() ? OK : DATA_NOT_AVAILABLE;
                } else {
                    // прогноз не был успешен, делаем поиск котировки
                    if(set_start_forecast(first_timestamp_day)) {
                        day_data = array_daily_data[index_forecast_array].first;
                        return !day_data.empty() ? OK : DATA_NOT_AVAILABLE;
                    }
                    // поиск не дал результатов, грузим котировки
                    read_daily_data(first_timestamp_day, indent_timestamp_past, indent_timestamp_future);
                    if(set_start_forecast(first_timestamp_day)) {
                        day_data = array_daily_data[index_forecast_array].first;
                        return !day_data.empty() ? OK : DATA_NOT_AVAILABLE;
                    }
                    return STRANGE_PROGRAM_BEHAVIOR;
                }
            }
            return INVALID_PARAMETER;
        }

        /** \brief Получить данные за указанный день
         *
         * \warning Данный метод не защищает от подглядывания в будущее!
         * \param timestamp Метка времени
         * \param optimization Тип оптимизации доступа к данным
         * \return Вернет данные за указанный день
         */
        T &get_day_data(
                const xtime::timestamp_t timestamp,
                const int optimization = xquotes_common::OPTIMIZATION_SEQUENTIAL_READING) {
            if(optimization == xquotes_common::WITHOUT_OPTIMIZATION) {
                return get_link_day_data(timestamp);
            } else
            if(optimization == xquotes_common::OPTIMIZATION_SEQUENTIAL_READING) {
                const xtime::timestamp_t first_timestamp_day = xtime::get_first_timestamp_day(timestamp);
                if( array_daily_data.size() > 0 &&
                    array_daily_data[index_forecast_array].second == first_timestamp_day) {
                    const uint32_t index = index_forecast_array;
                    make_next_forecast();
                    return array_daily_data[index].first;
                } else {
                    // прогноз не был успешен, делаем поиск котировки
                    if(set_start_forecast(first_timestamp_day)) {
                        return array_daily_data[index_forecast_array].first;
                    }
                    // поиск не дал результатов, грузим котировки
                    read_daily_data(first_timestamp_day, indent_timestamp_past, indent_timestamp_future);
                    if(set_start_forecast(first_timestamp_day)) {
                        return array_daily_data[index_forecast_array].first;
                    }
                    return null_day_data;
                }
            }
            return null_day_data;
        }

        /** \brief Проверить наличие подфайла по метке времени
         * \param timestamp метка времени
         * \return вернет true если файл есть
         */
        bool check_timestamp(const xtime::timestamp_t timestamp) {
            return check_subfile(xtime::get_day(timestamp));
        }

        /** \brief Узнать максимальную и минимальную метку времени подфайлов
         * \param min_timestamp Метка времени в начале дня начала исторических данных
         * \param max_timestamp Метка времени в начале дня конца исторических данных
         * \return Вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int get_min_max_timestamp(xtime::timestamp_t &min_timestamp, xtime::timestamp_t &max_timestamp) const {
            xquotes_common::key_t min_key = 0, max_key = 0;
            int err = get_min_max_key(min_key, max_key);
            min_timestamp = min_key * xtime::SECONDS_IN_DAY;
            max_timestamp = max_key * xtime::SECONDS_IN_DAY;
            return err;
        }

        ~DailyDataStorage() {
            close();
            if(is_mem_dict_file) delete [] dictionary_file_buffer;
        };
	};

	typedef DailyDataStorage<std::string> StringStorage;    /**< Хранилище строк */
}

#endif // XQUOTES_DAILY_DATA_STORAGE_HPP_INCLUDED
