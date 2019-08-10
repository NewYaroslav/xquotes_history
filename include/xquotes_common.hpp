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
    typedef unsigned long key_t;
    typedef unsigned long link_t;
    typedef unsigned long price_t;

    const double PRICE_MULTIPLER = 100000.0d;   ///< множитель для 5-ти значных котировок
    const int MINUTES_IN_DAY = 1440;            ///< количество минут в одном дне

    /** \brief Перевести double-цену в тип price_t
     * \param price цена
     * \return цена, представленная в price_t
     */
    inline price_t convert_to_uint(double price) {
        return (price_t)((price * PRICE_MULTIPLER) + 0.5);
    }

    /** \brief Перевести price_t-цену в тип double
     * \param price цена
     * \return цена, представленная в double
     */
    inline double convert_to_double(price_t price) {
        return (double)price / PRICE_MULTIPLER;
    }

    class Candle {
    public:
        double open = 0;
        double high = 0;
        double low = 0;
        double close = 0;
        double volume = 0;
        unsigned long long timestamp = 0;
        Candle() {};

        Candle(double open, double high, double low, double close, unsigned long long timestamp) {
            Candle::open = open;
            Candle::high = high;
            Candle::low = low;
            Candle::close = close;
            Candle::timestamp = timestamp;
        }

        Candle(double open, double high, double low, double close, double volume, unsigned long long timestamp) {
            Candle::open = open;
            Candle::high = high;
            Candle::low = low;
            Candle::close = close;
            Candle::volume = volume;
            Candle::timestamp = timestamp;
        }
    };

    /// Набор вариантов оптимизаций
    enum {
        WITHOUT_OPTIMIZATION = 0,               ///< Без оптимизация
        OPTIMIZATION_SEQUENTIAL_READING = 1,    ///< Оптимизация последовательного чтения минутных свечей
    };

    /// Константы для настроек
    enum {
        PRICE_CLOSE = 0,    ///< Использовать цену закрытия
        PRICE_OPEN = 1,     ///< Использовать цену открытия
        ALL_PRICE = 2,      ///< Использовать все цены свечи
        BUY = 1,            ///< Сделка на покупку
        SELL = -1,          ///< Сделка на продажу
        WIN = 1,            ///< Удачная сделка, победа
        LOSS = -1,          ///< Убычтоная сделка, проигрыш
        NEUTRAL = 0,        ///< Нейтральный результат
    };

    /// Набор возможных состояний ошибки
    enum {
        OK = 0,                         ///< Ошибок нет
        NO_INIT = -1,
        INVALID_PARAMETER = -6,         ///< Один из параметров неверно указан
        DATA_NOT_AVAILABLE = -7,        ///< Данные не доступны
        INVALID_ARRAY_LENGH = -1,       ///< Неправильная длина массиа
        NOT_OPEN_FILE = -9,             ///< Файл не был открыт
        NOT_WRITE_FILE = -10,           ///< Файл нельзя записать
        NOT_COMPRESS_FILE = -11,        ///< Файл нельзя сжать
        NOT_DECOMPRESS_FILE = -12,      ///< Файл нельзя разорхивировать
        DATA_SIZE_ERROR = -13,          ///< Ошибка размера данных
        FILE_CANNOT_OPENED = -15,       ///< Файл не может быть открыт
        FILE_CANNOT_RENAMED = -16,       ///< Файл не может быть переименован
        FILE_CANNOT_REMOVED = -17,       ///< Файл не может быть переименован
        FILE_NOT_OPENED = -18,
        NO_SUBFILES = -19,
        SUBFILES_NOT_FOUND = -20,
        SUBFILES_COMPRESSION_ERROR = -21,       ///< Ошибка сжатия подфайла
        SUBFILES_DECOMPRESSION_ERROR = -22,     ///< Ошибка декомпрессии подфайла
        STRANGE_PROGRAM_BEHAVIOR = -23,
    };
}

#endif // XQUOTES_COMMON_HPP_INCLUDED
