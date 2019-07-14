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
