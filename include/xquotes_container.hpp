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

/** \file Файл с шаблоном контейнера для хранения данных
 * \brief Контейнер подойдет для хранения промежуточных данных вычислений
 */
#ifndef XQUOTES_CONTAINER_HPP_INCLUDED
#define XQUOTES_CONTAINER_HPP_INCLUDED

#include "xquotes_common.hpp"
#include "xtime.hpp"
//#include <array>
#include <vector>
#include <memory>
#include <algorithm>

namespace xquotes_container {
    using namespace xquotes_common;

    /** \brief Класс контейнера для хранения данных
     * \details Данный класс можно использовать для реализации контейнера данных.
     * Это может пригодиться, например, для хранения промежуточных данных оптимизатора стратегий.
     * \warning Класс объекта для хранения в контейнере должен иметь объект timestamp для хранения метки времени!
     */
	template<typename T>
	class Container {
    private:
        std::vector<std::shared_ptr<T>> data;

        /** \brief Сортировка данных
         */
        void sort_data() {
            if(!std::is_sorted(data.begin(), data.end(), [](const std::shared_ptr<T> &a, const std::shared_ptr<T> &b) {
                        return a->timestamp < b->timestamp;
                    })) {
                std::sort(data.begin(), data.end(), [](const std::shared_ptr<T> &a, const std::shared_ptr<T> &b) {
                    return a->timestamp < b->timestamp;
                });;
            }
        }

    public:

        Container() {};

        ~Container() {};

        /** \brief Получитть объект по метке времени
         * \param object Объект
         * \param timestamp Метка времени
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int get(T& object, const xtime::timestamp_t &timestamp) {
            auto data_it = std::lower_bound(
                data.begin(),
                data.end(),
                timestamp,
                [](const std::shared_ptr<T> &lhs, const xtime::timestamp_t &timestamp) {
                return lhs->timestamp < timestamp;
            });
            if(data_it == data.end()) {
                return DATA_NOT_AVAILABLE;
            } else
            if((*data_it).get()->timestamp == timestamp) {
                object = **data_it;
                return OK;
            }
            return INVALID_PARAMETER;
        }

        /** \brief Получить указатель на объект (если есть)
         * \param timestamp Метка времени
         * \return Вернет указатель на объект или ноль
         */
        T* get(const xtime::timestamp_t &timestamp) {
            auto data_it = std::lower_bound(
                data.begin(),
                data.end(),
                timestamp,
                [](const std::shared_ptr<T> &lhs, const xtime::timestamp_t &timestamp) {
                return lhs->timestamp < timestamp;
            });
            if(data_it == data.end()) {
                return NULL;
            } else
            if((*data_it).get()->timestamp == timestamp) {
                return (**data_it).get();
            }
            return NULL;
        }

        /** \brief Добавить объект в контейнер
         * \param object Объект
         * \param timestamp Метка времени объекта
         */
        void add(T& object, const xtime::timestamp_t &timestamp) {
            if(data.size() == 0) {
                object.timestamp = timestamp;
                data.push_back(std::make_shared<T>(object));
                return;
            }
            auto data_it = std::lower_bound(
                data.begin(),
                data.end(),
                timestamp,
                [](const std::shared_ptr<T> &lhs, const xtime::timestamp_t &timestamp) {
                return lhs->timestamp < timestamp;
            });
            if(data_it == data.end()) {
                object.timestamp = timestamp;
                data.push_back(std::make_shared<T>(object));
            } else
            if((*data_it).get()->timestamp == timestamp) {
                object.timestamp = timestamp;
                **data_it = object;
            }
            sort_data();
        }

        /** \brief Добавить объект в контейнер
         * \param object Объект
         */
        void add(T& object) {
            if(data.size() == 0) {
                data.push_back(std::make_shared<T>(object));
                return;
            }
            auto data_it = std::lower_bound(
                data.begin(),
                data.end(),
                object.timestamp,
                [](const std::shared_ptr<T> &lhs, const xtime::timestamp_t &timestamp) {
                return lhs->timestamp < timestamp;
            });
            if(data_it == data.end()) {
                data.push_back(std::make_shared<T>(object));
            } else
            if((*data_it).get()->timestamp == object.timestamp) {
                **data_it = object;
            }
            sort_data();
        }

        /** \brief Проверить наличие объекта  по метке времени
         * \param timestamp метка времени
         * \return вернет true если файл есть
         */
        bool check_timestamp(const xtime::timestamp_t &timestamp) {
            if(data.size() == 0) return false;
            auto data_it = std::lower_bound(
                data.begin(),
                data.end(),
                timestamp,
                [](const std::shared_ptr<T> &lhs, const xtime::timestamp_t &timestamp) {
                return lhs->timestamp < timestamp;
            });
            if(data_it == data.end()) return false;
            return true;
        }

        /** \brief Узнать максимальную и минимальную метку времени
         * \param min_timestamp метка времени в начале дня начала исторических данных
         * \param max_timestamp метка времени в начале дня конца исторических данных
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int get_min_max_timestamp(xtime::timestamp_t &min_timestamp, xtime::timestamp_t &max_timestamp) const {
            if(data.size() == 0) {
                min_timestamp = 0;
                max_timestamp = 0;
                return DATA_NOT_AVAILABLE;
            }
            min_timestamp = data[0]->timestamp;
            max_timestamp = data.back()->timestamp;
        }

        /** \brief Установить новую метку времени
         * \details Данный метод найдет объект с меткой времени old_timestamp и поменяет его метку времени на new_timestamp
         * \param old_timestamp Метка времени, которую надо заменить
         * \param new_timestamp Новая метка времени
         */
        void set_new_timestamp(const xtime::timestamp_t &old_timestamp, const xtime::timestamp_t &new_timestamp) {
            auto data_it = std::lower_bound(
                data.begin(),
                data.end(),
                old_timestamp,
                [](const std::shared_ptr<T> &lhs, const xtime::timestamp_t &timestamp) {
                return lhs->timestamp < timestamp;
            });
            if(data_it == data.end()) return;
            (**data_it).timestamp = new_timestamp;
            sort_data();
        }

        /** \brief Добавить объект с удалением старых данных
         * \param object Объект
         * \param max_object Максимальное количество объектов в контейнере
         */
        void add_with_del_old(T& object, const size_t max_object) {
            if(data.size() >= max_object) {
                // удаляем старые объекты
                while(data.size() > max_object) {
                    data.erase(data.begin());
                }
                *data[0] = object;
                sort_data();
            } else {
                add(object);
            }
        }
    };
}

#endif // XQUOTES_CONTAINER_HPP_INCLUDED
