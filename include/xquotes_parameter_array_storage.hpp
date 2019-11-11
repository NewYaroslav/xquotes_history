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

/** \file Файл с классами для хранения массивов параметров
 * \brief Данный файл содержит классы для хранения массивов параметров
 */
#ifndef XQUOTES_PARAMETER_ARRAY_STORAGE_HPP_INCLUDED
#define XQUOTES_PARAMETER_ARRAY_STORAGE_HPP_INCLUDED

#include "xquotes_daily_data_storage.hpp"
#include <iostream>

namespace xquotes_parameter_array_storage {
    using namespace xquotes_common;

    /** \brief Базовый класс для хранения параметров
     */
	class ParameterArrays {
    public:
        std::unique_ptr<uint8_t[]> byte_array;  /**< Массив с данными */
        size_t byte_array_size = 0;
        uint8_t *byte_array_ptr = nullptr;

        ParameterArrays() {}

        virtual ParameterArrays & operator = (const ParameterArrays &parameter_arrays) {
            if(this != &parameter_arrays) {
                if(byte_array_size != parameter_arrays.byte_array_size) {
                    byte_array_size = parameter_arrays.byte_array_size;
                    byte_array = std::unique_ptr<uint8_t[]>(new uint8_t[byte_array_size]);
                }
                uint8_t *dst_data = byte_array.get();
                uint8_t *src_data = parameter_arrays.byte_array.get();
                std::copy(src_data, src_data + byte_array_size, dst_data);
                byte_array_ptr = byte_array.get();
            }
            return *this;
        }

        ParameterArrays(const ParameterArrays &parameter_arrays) {
            if(this != &parameter_arrays) {
                if(byte_array_size != parameter_arrays.byte_array_size) {
                    byte_array_size = parameter_arrays.byte_array_size;
                    byte_array = std::unique_ptr<uint8_t[]>(new uint8_t[byte_array_size]);
                }
                uint8_t *dst_data = byte_array.get();
                uint8_t *src_data = parameter_arrays.byte_array.get();
                std::copy(src_data, src_data + byte_array_size, dst_data);
                byte_array_ptr = byte_array.get();
            }
        }

        virtual bool empty() {
            return (byte_array_size == 0);
        }

        virtual void assign(const char* s, size_t n) {
            if(n != byte_array_size) {
                byte_array = std::unique_ptr<uint8_t[]>(new uint8_t[n]);
                byte_array_size = n;
            }
            char *point = (char*)byte_array.get();
            std::copy(s, s + n, point);
            byte_array_ptr = byte_array.get();
        }

        virtual size_t size() const {
            return byte_array_size;
        }

        virtual const char* data() const noexcept {
            return (const char*)byte_array.get();
        }
	};

	template <class T, size_t COLUMNS, size_t ROWS = 0>
	class Parameter2D : public ParameterArrays {
    public:
        static_assert(
            !std::is_same<T, float>::value ||
            !std::is_same<T, double>::value ||
            !std::is_same<T, int8_t>::value ||
            !std::is_same<T, int16_t>::value ||
            !std::is_same<T, int32_t>::value ||
            !std::is_same<T, int64_t>::value ||
            !std::is_same<T, uint8_t>::value ||
            !std::is_same<T, uint16_t>::value ||
            !std::is_same<T, uint32_t>::value ||
            !std::is_same<T, uint64_t>::value,
            "ERROR: T must be a base data type"
        );
        Parameter2D() : ParameterArrays() {
            if(ROWS != 0 && COLUMNS != 0) {
                byte_array_size = ROWS * COLUMNS * sizeof(T);
                byte_array = std::unique_ptr<uint8_t[]>(new uint8_t[byte_array_size]);
                byte_array_ptr = byte_array.get();
                std::fill(byte_array_ptr, byte_array_ptr + byte_array_size, '\0');
            }
        }

        virtual void set_rows(const size_t number_rows) {
            const size_t old_size = byte_array_size;
            const size_t new_size = number_rows * COLUMNS * sizeof(T);
            if(new_size != byte_array_size) {
                byte_array = std::unique_ptr<uint8_t[]>(new uint8_t[new_size]);
                byte_array_size = new_size;
                byte_array_ptr = byte_array.get();
                std::fill(byte_array_ptr + old_size, byte_array_ptr + byte_array_size, '\0');
            }
        }

        virtual size_t get_rows() const {
            return (byte_array_size / sizeof(T)) / COLUMNS;
        }

        virtual size_t get_columns() const {
            return COLUMNS;
        }

        Parameter2D(const size_t number_rows) : ParameterArrays() {
            set_rows(number_rows);
        }

        virtual void assign(const char* s, size_t n) {
            if(n != byte_array_size) {
                byte_array = std::unique_ptr<uint8_t[]>(new uint8_t[n]);
                byte_array_size = n;
            }
            char *point = (char*)byte_array.get();
            std::copy(s, s + n, point);
            byte_array_ptr = byte_array.get();
        }

        virtual T &get(const size_t x, const size_t y) {
            T *value_ptr = (T*)byte_array_ptr;
            const size_t index = y * COLUMNS + x;
            return value_ptr[index];
        }

        virtual void set(const T &value, const size_t x, const size_t y) {
            T *value_ptr = (T*)byte_array_ptr;
            const size_t index = y * COLUMNS + x;
            value_ptr[index] = value;
        }
	};
}

#endif // XQUOTES_PARAMETER_ARRAY_STORAGE_HPP_INCLUDED
