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

/** \file Файл с общими константами, перечислениями и т.д.
 * \brief Данный файл содержит общие константы, перечисления и т.д.
 * Файл содержит класс Candle необходим для хранения бара \ японской свечи
 * Функця get_decimal_places нужна для определения числа знаков после запятой
 */
#ifndef XQUOTES_COMMON_HPP_INCLUDED
#define XQUOTES_COMMON_HPP_INCLUDED

#include <cstdlib>
#include <cmath>

namespace xquotes_common {
    typedef unsigned short key_t;
    typedef unsigned long link_t;
    typedef unsigned long price_t;
    typedef unsigned long note_t;

    const double PRICE_MULTIPLER = 100000.0d;   ///< множитель для 5-ти значных котировок
    const int MINUTES_IN_DAY = 1440;            ///< количество минут в одном дне

    const unsigned long ONLY_ONE_PRICE_BUFFER_SIZE = MINUTES_IN_DAY * sizeof(price_t);
    const unsigned long CANDLE_WITHOUT_VOLUME_BUFFER_SIZE = MINUTES_IN_DAY * 4 * sizeof(price_t);
    const unsigned long CANDLE_WITH_VOLUME_BUFFER_SIZE = MINUTES_IN_DAY * 5 * sizeof(price_t);

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

    /** \brief Класс для хранения японских свечей
     */
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
        PRICE_CLOSE = 0,            ///< Использовать цену закрытия
        PRICE_OPEN = 1,             ///< Использовать цену открытия
        PRICE_OHLC = 2,             ///< Использовать все цены свечи без объемов
        PRICE_OHLCV = 3,            ///< Использовать все цены свечи с объемом
        PRICE_LOW = 4,              ///< Использовать наименьшую цену свечи
        PRICE_HIGH = 5,             ///< Использовать наибольшую цену свечи
        BUY = 1,                    ///< Сделка на покупку
        SELL = -1,                  ///< Сделка на продажу
        WIN = 1,                    ///< Удачная сделка, победа
        LOSS = -1,                  ///< Убычтоная сделка, проигрыш
        NEUTRAL = 0,                ///< Нейтральный результат
        USE_COMPRESSION = 1,        ///< Использовать сжатие файлов
        DO_NOT_USE_COMPRESSION = 0, ///< Не использовать сжатие файлов
        DO_NOT_CHANGE_TIME_ZONE = 0,///< Не менять часовой пояс
        CET_TO_GMT = 1,             ///< Поменять часовой пояс с CET на GMT
        EET_TO_GMT = 2,             ///< Поменять часовой пояс с EET на GMT
        GMT_TO_CET = 3,             ///< Поменять часовой пояс с GMT на CET
        GMT_TO_EET = 4,             ///< Поменять часовой пояс с GMT на EET
        SKIPPING_BAD_CANDLES = 0,   ///< Пропускать бары или свечи с отсутствующими данными
        FILLING_BAD_CANDLES = 1,    ///< Заполнять бары или свечи с отсутствующими данными предыдущим значением
        WRITE_BAD_CANDLES = 2,      ///< Записывать как есть бары или свечи с отсутствующими данными
    };

    /// Набор возможных состояний ошибки
    enum {
        OK = 0,                         ///< Ошибок нет
        NO_INIT = -1,                   ///< Нет инициализаци
        INVALID_PARAMETER = -6,         ///< Один из параметров неверно указан
        DATA_NOT_AVAILABLE = -7,        ///< Данные не доступны
        INVALID_ARRAY_LENGH = -8,       ///< Неправильная длина массиа
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

#ifdef XQUOTES_USE_DICTIONARY_CURRENCY_PAIR
    /** \brief Получить тип цены с указанием специфики
     * \param price_type Тип цены (данную функицю имеет смысл использовать с PRICE_OHLC)
     * \param currency_pair Номер валютной пары (0 - нет специфики)
     * \return Тип цены с указанием специфики
     */
    inline int get_price_type_with_specific(const int price_type, const int currency_pair) {
        return (price_type & 0x0F) | ((currency_pair << 8) & 0xFF00);
    }
#endif // XQUOTES_USE_DICTIONARY_CURRENCY_PAIR

    /** \brief Сравнить две цены с заданной точностью
     * \param a Сравниваемая цена
     * \param b Сравниваемая цена
     * \param factor Множитель точности, указывать число кратное 10 (10,100,1000 и т.д.)
     * \return Вернет true если цены равны
     */
    inline bool compare_price(const double a, const double b, const int factor) {
        const double MAX_DIFF = 1.0d/ (const double)(factor * (const int)10);
        if(std::abs(a - b) < MAX_DIFF) return true;
        return false;
    }

    /** \brief Получить дробную часть
     * \param price Цена
     * \param precision Точность, указатьколичество знаков после запятой (если is_factor = false) или множитель
     * \param is_factor Если true то функция принимает множитель
     * \return Вернет дробную часть
     */
    inline int get_fractional(const double price, const int precision, const bool is_factor = false) {
        if(is_factor) return (int)((price - std::floor(price)) * precision + 0.5);
        else return (int)((price - std::floor(price)) * std::pow(10, precision) + 0.5);
    }

    /** \brief Огрубление цены
     * \param price Цена
     * \param precision Точность, указывать число кратное 10
     * \param is_factor Если true то функция принимает множитель
     * \return Цена после огрубления
     */
    inline double coarsening_price(const double price, const int precision, const bool is_factor = false) {
        if(is_factor) {
            const int temp = price * (double)precision + 0.5;
            return (double) temp / (double) precision;
        } else {
            int factor = std::pow(10, precision);
            const int temp = price * (double)factor + 0.5;
            return (double) temp / (double) factor;
        }
    }

    /** \brief Получить число десятичных знаков после запятой
     * \param input Входные данные
     * \param is_factor Если true то функция возвращает множитель
     * \return Количество знаков после запятой или множитель
     */
    template<class T>
    int get_decimal_places(T& input, const bool is_factor = false) {
        int factor = 0, num = 1;
        for(int n = 10; n < 1000000; n *= 10, num++) {
            bool is_found = true;
            factor = n;
            for(int i = 0; i < (int)input.size(); ++i) {
                if(
                (input[i].close != 0 && !compare_price(input[i].close, coarsening_price(input[i].close, n, true), n)) ||
                (input[i].open != 0 && !compare_price(input[i].open, coarsening_price(input[i].open, n, true), n)) ||
                (input[i].low != 0 && !compare_price(input[i].low, coarsening_price(input[i].low, n, true), n)) ||
                (input[i].high != 0 && !compare_price(input[i].high, coarsening_price(input[i].high, n, true), n))) {
                    is_found = false;
                    break;
                }
            }
            if(is_found) {
                break;
            }
        }
        if(is_factor) return factor;
        else return num;
    }
}

#endif // XQUOTES_COMMON_HPP_INCLUDED
