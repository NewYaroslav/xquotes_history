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

/** \file Файл с классом для работы с JSON
 * \brief Данный файл содержит класс для работы с JSON
 */
#ifndef XQUOTES_JSON_STORAGE_HPP_INCLUDED
#define XQUOTES_JSON_STORAGE_HPP_INCLUDED

#include "xquotes_daily_data_storage.hpp"
#include <nlohmann/json.hpp>

namespace xquotes_json_storage {
    using namespace xquotes_common;
    using json = nlohmann::json;

#if(1)
    /** \brief Класс для работы с JSON
     */
	class JsonStorage : public xquotes_daily_data_storage::StringStorage {
    private:
        json json_null;
    public:
        JsonStorage() : DailyDataStorage() {};

        JsonStorage(
            const std::string &path,
            const char* dictionary_ptr = NULL,
            const size_t dictionary_size = 0) : DailyDataStorage(path, dictionary_ptr, dictionary_size) {
        };

        /** \brief Записать данные JSON за день
         * \param j Данные JSON одного дня
         * \param timestamp Метка времени начала дня указанных данных
         * \return Вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int write_json(json &j, const xtime::timestamp_t timestamp) {
            return write_day_data(j.dump(), timestamp);
        }

        int get_json(
                json &j,
                const xtime::timestamp_t timestamp,
                const int optimization = xquotes_common::OPTIMIZATION_SEQUENTIAL_READING) {
            std::string temp;
            int err = get_day_data(temp, timestamp, optimization);
            if(err != OK) return err;
            try {
                j = json::parse(temp);
            }
            catch(...) {
                j = json_null;
                return xquotes_common::DATA_NOT_AVAILABLE;
            }
            return xquotes_common::OK;
        }

        json get_json(
                const xtime::timestamp_t timestamp,
                const int optimization = xquotes_common::OPTIMIZATION_SEQUENTIAL_READING) {
            std::string temp;
            int err = get_day_data(temp, timestamp, optimization);
            if(err != OK) return json_null;
            try {
                return json::parse(temp);
            }
            catch(...) {
                return json_null;
            }
        }
	};
#endif
}

#endif // XQUOTES_JSON_STORAGE_HPP_INCLUDED
