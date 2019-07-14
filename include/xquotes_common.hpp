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
#ifndef XQUOTES_COMMON_HPP_INCLUDED
#define XQUOTES_COMMON_HPP_INCLUDED

namespace xquotes_common {
    const double PRICE_MULTIPLER = 100000.0d;   ///< множитель для 5-ти значных котировок
    const int MINUTES_IN_DAY = 1440;            ///< количество минут в одном дне

    /// Набор возможных состояний ошибки
    enum {
        OK = 0,                         ///< Ошибок нет
        INVALID_PARAMETER = -6,         ///< Один из параметров неверно указан
        DATA_NOT_AVAILABLE = -7,        ///< Данные не доступны
        INVALID_ARRAY_LENGH = -1,       ///< Неправильная длина массиа
        NOT_OPEN_FILE = -9,             ///< Файл не был открыт
        NOT_WRITE_FILE = -10,           ///< Файл нельзя записать
        NOT_COMPRESS_FILE = -11,        ///< Файл нельзя сжать
        NOT_DECOMPRESS_FILE = -12,      ///< Файл нельзя разорхивировать
        DATA_SIZE_ERROR = -13,          ///< Ошибка размера данных
        FILE_CANNOT_OPENED = -15,       ///< Файл не может быть открыт
    };
}

#endif // XQUOTES_COMMON_HPP_INCLUDED
