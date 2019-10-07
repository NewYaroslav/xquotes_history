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

/** \file Файл с функциями для работы с библиотекой zstd
 * \brief Данный файл содержит полезные функции для работы с библиотекой zstd
 * Это в основном функция train_zstd для обучения алгоритма сжатия
 * Остальные функции не востребованы и унаследованы путем ctrl+alt
 */
#ifndef XQUOTES_ZSTDEASY_HPP_INCLUDED
#define XQUOTES_ZSTDEASY_HPP_INCLUDED

#include "xquotes_storage.hpp"
#include "banana_filesystem.hpp"
#include "dictBuilder/zdict.h"
#include "zstd.h"
#include <cstring>

namespace xquotes_zstd {
    using namespace xquotes_common;

    std::string convert_hex_to_string(unsigned char value) {
        char hex_string[32] = {};
        sprintf(hex_string,"0x%.2X", value);
        return std::string(hex_string);
    }

    std::string str_toupper(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c){ return std::toupper(c); });
        return s;
    }

    std::string str_tolower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c){ return std::tolower(c); });
        return s;
    }

    /** \brief Тренируйте словарь из массива образцов
     * \param storage Хранилище данных
     * \param subfiles_list Список ключей подфайлов
     * \param path путь к файлу словаря
     * \param name имя словаря
     * \param dict_buffer_capacit размер словаря
     * \param is_file Флаг файла словаря. Если установлен, то словарь будет сохранен как бинарный файл
     * \return венет 0 в случае успеха
     */
    int train_zstd(
            xquotes_storage::Storage &storage,
            const std::vector<int> &subfiles_list,
            const std::string &path,
            const std::string &name,
            const size_t dict_buffer_capacit = 102400,
            const bool is_file = true) {

        // буферы для подготовки образцов обучения
        size_t all_subfiles_size = 0;
        void *samples_buffer = NULL;
        size_t num_subfiles = 0;
        size_t *samples_size = NULL;

        // выясним, сколько нам нужно выделить памяти под образцы
        unsigned long subfiles_size = 0;
        unsigned long subfiles_counter = 0;
        for(size_t i = 0; i < subfiles_list.size(); ++i) {
            unsigned long subfile_size = 0;
            int err_stor = storage.get_subfile_size(subfiles_list[i], subfile_size);
            if(err_stor == OK) {
                subfiles_size += subfile_size;
                subfiles_counter++;
            }
        }
        // выделим память под образцы
        samples_buffer = malloc(subfiles_size);
        samples_size = (size_t*)malloc(subfiles_counter * sizeof(size_t));

        // заполним буферы образцов
        for(size_t i = 0; i < subfiles_list.size(); ++i) {
            unsigned long subfile_size = 0;
            int err_stor = storage.get_subfile_size(subfiles_list[i], subfile_size);
            if(err_stor == OK && subfile_size > 0) {
                num_subfiles++;
                size_t start_pos = all_subfiles_size;
                all_subfiles_size += subfile_size;

                samples_size[num_subfiles - 1] = subfile_size;
                char *point_buffer = (char*)samples_buffer + start_pos;
                int err_read = storage.read_subfile(subfiles_list[i], point_buffer, subfile_size);

                if(err_read != OK) {
                    std::cout << "read subfile error, subfile: " << subfiles_list[i] << " code: " << err_read << std::endl;
                    if(samples_buffer != NULL) free(samples_buffer);
                    if(samples_size != NULL) free(samples_size);
                    return NOT_OPEN_FILE;
                } else {
                    std::cout << "subfile: " << subfiles_list[i] << " #" << num_subfiles << "/" << subfiles_list.size() << "\r";
                }
            } else {
                std::cout << "storage error, subfile: " << subfiles_list[i] << " code: " << err_stor << std::endl;
                if(samples_buffer != NULL) free(samples_buffer);
                if(samples_size != NULL) free(samples_size);
                return NOT_OPEN_FILE;
            }
        }
        std::cout << std::endl;

        void *dict_buffer = NULL;
        dict_buffer = malloc(dict_buffer_capacit);
        memset(dict_buffer, 0, dict_buffer_capacit);
        size_t dictionary_size = ZDICT_trainFromBuffer(dict_buffer, dict_buffer_capacit, samples_buffer, samples_size, num_subfiles);
        if(ZDICT_isError(dictionary_size)) {
            std::cout << "zstd error: " << ZDICT_getErrorName(dictionary_size) << std::endl;
            return DATA_NOT_AVAILABLE;
        }

        if(is_file) {
            size_t err = bf::write_file(path, dict_buffer, dictionary_size);
            return err > 0 ? OK : NOT_WRITE_FILE;
        } else {
            std::string dictionary_name_upper = str_toupper(name);
            std::string dictionary_name_lower = str_tolower(name);

            // сохраним как с++ файл
            unsigned char *dict_buffer_point = (unsigned char *)dict_buffer;

            std::string out;
            out += "#ifndef XQUOTES_DICTIONARY_" + dictionary_name_upper+ "_HPP_INCLUDED\n";
            out += "#define XQUOTES_DICTIONARY_" + dictionary_name_upper + "_HPP_INCLUDED\n";
            out += "\n";
            out += "namespace zstd_dictionary {\n";
            out += "\tconst static unsigned char ";
            out += dictionary_name_lower;
            out += "[";
            out += std::to_string(dictionary_size);
            out += "] = {\n";
            out += "\t\t";
            for(size_t j = 0; j < dictionary_size; ++j) {
                if(j > 0 && (j % 16) == 0) {
                    out += "\n\t\t";
                }
                out += convert_hex_to_string(dict_buffer_point[j]);
                out += ",";
                if(j == dictionary_size - 1) {
                    out += "\n\t};\n";
                }
            }
            out += "}\n";
            out += "#endif // XQUOTES_DICTIONARY_" + dictionary_name_upper + "_HPP_INCLUDED\n";
            std::string path_out = bf::set_file_extension(path, ".hpp");
            size_t err = bf::write_file(path_out, (void*)out.c_str(), out.size());
            return err > 0 ? OK : NOT_WRITE_FILE;
        }
    }

    /** \brief Тренируйте словарь из массива образцов
     * \param files_list Список файлов для обучения
     * \param path путь к файлу словаря
     * \param name имя словаря
     * \param dict_buffer_capacit размер словаря
     * \param is_file Флаг файла словаря. Если установлен, то словарь будет сохранен как бинарный файл
     * \return венет 0 в случае успеха
     */
    int train_zstd(
            const std::vector<std::string> &files_list,
            const std::string &path,
            const std::string &name,
            const size_t &dict_buffer_capacit = 102400,
            const bool is_file = true) {

        // буферы для подготовки образцов обучения
        size_t all_files_size = 0;
        void *samples_buffer = NULL;
        size_t num_files = 0;
        size_t *samples_size = NULL;

        // выясним, сколько нам нужно выделить памяти под образцы
        unsigned long files_size = 0;
        unsigned long files_counter = 0;
        for(size_t i = 0; i < files_list.size(); ++i) {
            unsigned long file_size = bf::get_file_size(files_list[i]);
            if(file_size > 0) {
                files_size += file_size;
                files_counter++;
            }
        }

        // выделим память под образцы
        samples_buffer = malloc(files_size);
        samples_size = (size_t*)malloc(files_counter * sizeof(size_t));

        // заполним буферы образцов
        for(size_t i = 0; i < files_list.size(); ++i) {
            unsigned long file_size = bf::get_file_size(files_list[i]);
            if(file_size > 0) {
                num_files++;
                size_t start_pos = all_files_size;
                all_files_size += file_size;
                samples_size[num_files - 1] = file_size;

                unsigned long err = bf::load_file(files_list[i], samples_buffer, all_files_size, start_pos);
                if(err != file_size) {
                    std::cout << "read file error, file: " << files_list[i] << " size: " << err << std::endl;
                    if(samples_buffer != NULL)
                        free(samples_buffer);
                    if(samples_size != NULL)
                        free(samples_size);
                    return NOT_OPEN_FILE;
                } else {
                    std::cout << "file: " << files_list[i] << " #" << num_files << "/" << files_list.size() << "\r";
                }
            } else {
                std::cout << "buffer size: get file size error, file: " << files_list[i] << " size: " << file_size << std::endl;
                if(samples_buffer != NULL)
                    free(samples_buffer);
                if(samples_size != NULL)
                    free(samples_size);
                return NOT_OPEN_FILE;
            }
        }
        std::cout << std::endl;

        void *dict_buffer = NULL;
        dict_buffer = malloc(dict_buffer_capacit);
        memset(dict_buffer, 0, dict_buffer_capacit);
        size_t file_size = ZDICT_trainFromBuffer(dict_buffer, dict_buffer_capacit, samples_buffer, samples_size, num_files);

        if(is_file) {
            size_t err = bf::write_file(path, dict_buffer, file_size);
            return err > 0 ? OK : NOT_WRITE_FILE;
        } else {
            std::string dictionary_name_upper = str_toupper(name);
            std::string dictionary_name_lower = str_tolower(name);

            // сохраним как с++ файл
            unsigned char *dict_buffer_point = (unsigned char *)dict_buffer;
            std::string out;
            out += "#ifndef XQUOTES_DICTIONARY_" + dictionary_name_upper+ "_HPP_INCLUDED\n";
            out += "#define XQUOTES_DICTIONARY_" + dictionary_name_upper + "_HPP_INCLUDED\n";
            out += "\n";
            out += "namespace zstd_dictionary {\n";
            out += "\tconst static unsigned char " + dictionary_name_lower + "[" + std::to_string(file_size) + "] = {\n";
            out += "\t\t";
            for(size_t j = 0; j < file_size; ++j) {
                if(j > 0 && (j % 16) == 0) {
                    out += "\n\t\t";
                }
                out += convert_hex_to_string(dict_buffer_point[j]) + ",";
                if(j == file_size - 1) {
                    out += "\n\t};\n";
                }
            }
            out += "}\n";
            out += "#endif // XQUOTES_DICTIONARY_" + dictionary_name_upper + "_HPP_INCLUDED\n";
            std::string path_out = bf::set_file_extension(path, ".hpp");
            size_t err = bf::write_file(path_out, (void*)out.c_str(), out.size());
            return err > 0 ? OK : NOT_WRITE_FILE;
        }
    }

    /** \brief Тренируйте словарь из массива образцов
    * \param path путь к файлам
    * \param file_name имя файл словаря, который будет сохранен по окончанию обучения
    * \param dict_buffer_capacit размер словаря
    * \return венет 0 в случае успеха
    */
    int train_zstd(
            std::vector<std::string> &files_list,
            std::string file_name,
            size_t dict_buffer_capacit = 102400) {
        size_t all_files_size = 0;
        void *samples_buffer = NULL;

        size_t num_files = 0;
        size_t *samples_size = NULL;
        for(size_t i = 0; i < files_list.size(); ++i) {
            int file_size = bf::get_file_size(files_list[i]);
            if(file_size > 0) {
                num_files++;
                size_t start_pos = all_files_size;
                all_files_size += file_size;
                std::cout << "buffer size: " << all_files_size << std::endl;
                samples_buffer = realloc(samples_buffer, all_files_size);
                samples_size = (size_t*)realloc((void*)samples_size, num_files * sizeof(size_t));
                samples_size[num_files - 1] = file_size;
                int err = bf::load_file(files_list[i], samples_buffer, all_files_size, start_pos);
                if(err != file_size) {
                    std::cout << "load file: error, " << files_list[i] << std::endl;
                    if(samples_buffer != NULL)
                        free(samples_buffer);
                    if(samples_size != NULL)
                        free(samples_size);
                    return NOT_OPEN_FILE;
                } else {
                    std::cout << "load file: " << files_list[i] << " #" << num_files << "/" << files_list.size() << std::endl;
                }
            } else {
                std::cout << "buffer size: error, " << files_list[i] << std::endl;
                if(samples_buffer != NULL)
                    free(samples_buffer);
                if(samples_size != NULL)
                    free(samples_size);
                return NOT_OPEN_FILE;
            }
        }
        void *dict_buffer = NULL;
        dict_buffer = malloc(dict_buffer_capacit);
        memset(dict_buffer, 0, dict_buffer_capacit);
        size_t file_size = ZDICT_trainFromBuffer(dict_buffer, dict_buffer_capacit, samples_buffer, samples_size, num_files);
        size_t err = bf::write_file(file_name, dict_buffer, file_size);
        return err > 0 ? OK : NOT_WRITE_FILE;
    }

    /** \brief Тренируйте словарь из массива образцов
     * \param path путь к файлам
     * \param file_name имя файл словаря, который будет сохранен по окончанию обучения
     * \return венет 0 в случае успеха
     */
    int train_zstd(std::string path, std::string file_name, size_t dict_buffer_capacit = 102400) {
        std::vector<std::string> files_list;
        bf::get_list_files(path, files_list, true);
        return train_zstd(files_list, file_name, dict_buffer_capacit);
    }

        /** \brief Сжать файл с использованием словаря
         * \param input_file файл, который надо сжать
         * \param output_file файл, в который сохраним данные
         * \param dictionary_file файл словаря для сжатия
         * \param compress_level уровень сжатия файла
         * \return вернет 0 в случае успеха
         */
    int compress_file(
            std::string input_file,
            std::string output_file,
            std::string dictionary_file,
            int compress_level = ZSTD_maxCLevel()) {
        int input_file_size = bf::get_file_size(input_file);
        if(input_file_size <= 0) return NOT_OPEN_FILE;

        int dictionary_file_size = bf::get_file_size(dictionary_file);
        if(dictionary_file_size <= 0) return NOT_OPEN_FILE;

        void *input_file_buffer = NULL;
        input_file_buffer = malloc(input_file_size);
        memset(input_file_buffer, 0, input_file_size);

        void *dictionary_file_buffer = NULL;
        dictionary_file_buffer = malloc(dictionary_file_size);
        memset(dictionary_file_buffer, 0, dictionary_file_size);

        bf::load_file(dictionary_file, dictionary_file_buffer, dictionary_file_size);
        bf::load_file(input_file, input_file_buffer, input_file_size);

        size_t compress_file_size = ZSTD_compressBound(input_file_size);
        void *compress_file_buffer = NULL;
        compress_file_buffer = malloc(compress_file_size);
        memset(compress_file_buffer, 0, compress_file_size);

        ZSTD_CCtx* const cctx = ZSTD_createCCtx();
        size_t compress_size = ZSTD_compress_usingDict(
            cctx,
            compress_file_buffer,
            compress_file_size,
            input_file_buffer,
            input_file_size,
            dictionary_file_buffer,
            dictionary_file_size,
            compress_level
            );

        if(ZSTD_isError(compress_size)) {
            std::cout << "error compressin: " << ZSTD_getErrorName(compress_size) << std::endl;
            ZSTD_freeCCtx(cctx);
            free(compress_file_buffer);
            free(dictionary_file_buffer);
            free(input_file_buffer);
            return NOT_COMPRESS_FILE;
        }

        bf::write_file(output_file, compress_file_buffer, compress_size);

        ZSTD_freeCCtx(cctx);
        free(compress_file_buffer);
        free(dictionary_file_buffer);
        free(input_file_buffer);
        return OK;
    }

    /** \brief Записать сжатый файл
     * \param file_name имя файла
     * \param dictionary_file файл словаря для сжатия
     * \param buffer буфер, который запишем
     * \param buffer_size размер буфера
     * \param compress_level уровень сжатия файла
     * \return вернет 0 в случае успеха
     */
    int write_compressed_file(
            std::string file_name,
            std::string dictionary_file,
            void *buffer,
            size_t buffer_size,
            int compress_level = ZSTD_maxCLevel()) {
        int dictionary_file_size = bf::get_file_size(dictionary_file);
        if(dictionary_file_size <= 0) return NOT_OPEN_FILE;

        void *dictionary_file_buffer = NULL;
        dictionary_file_buffer = malloc(dictionary_file_size);
        memset(dictionary_file_buffer, 0, dictionary_file_size);

        bf::load_file(dictionary_file, dictionary_file_buffer, dictionary_file_size);

        size_t compress_file_size = ZSTD_compressBound(buffer_size);
        void *compress_file_buffer = NULL;
        compress_file_buffer = malloc(compress_file_size);
        memset(compress_file_buffer, 0, compress_file_size);

        ZSTD_CCtx* const cctx = ZSTD_createCCtx();
        size_t compress_size = ZSTD_compress_usingDict(
            cctx,
            compress_file_buffer,
            compress_file_size,
            buffer,
            buffer_size,
            dictionary_file_buffer,
            dictionary_file_size,
            compress_level
            );

        if(ZSTD_isError(compress_size)) {
            std::cout << "error compressin: " << ZSTD_getErrorName(compress_size) << std::endl;
            ZSTD_freeCCtx(cctx);
            free(compress_file_buffer);
            free(dictionary_file_buffer);
            //free(input_file_buffer);
            return NOT_COMPRESS_FILE;
        }

        bf::write_file(file_name, compress_file_buffer, compress_size);

        ZSTD_freeCCtx(cctx);
        free(compress_file_buffer);
        free(dictionary_file_buffer);
        return OK;
    }

    /** \brief Записать сжатый файл
     * \param file_name имя файла
     * \param dictionary_file файл словаря для сжатия
     * \param prices котировки
     * \param times временные метки
     * \param compress_level уровень сжатия файла
     * \return вернет 0 в случае успеха
     */
    int write_binary_quotes_compressed_file(
            std::string file_name,
            std::string dictionary_file,
            std::vector<double> &prices,
            std::vector<unsigned long long> &times,
        int compress_level = ZSTD_maxCLevel()) {
        if(times.size() != prices.size())
            return DATA_SIZE_ERROR;
        size_t buffer_size = prices.size() * sizeof(double) +
            times.size() * sizeof(unsigned long long);
        void *buffer = NULL;
        buffer = malloc(buffer_size);
        std::memset(buffer, 0, buffer_size);
        size_t buffer_offset = 0;
        unsigned long data_size = times.size();
        std::memcpy((unsigned char*)buffer + buffer_offset, &data_size, sizeof(data_size));
        buffer_offset += sizeof(data_size);
        for(unsigned long i = 0; i < data_size; ++i) {
            std::memcpy((unsigned char*)buffer + buffer_offset, &prices[i], sizeof(double));
            buffer_offset += sizeof(double);
            std::memcpy((unsigned char*)buffer + buffer_offset, &times[i], sizeof(unsigned long long));
            buffer_offset += sizeof(unsigned long long);
        }
        int err = write_compressed_file(file_name, dictionary_file, buffer, buffer_size, compress_level);
        free(buffer);
        return err;
    }

    /** \brief Декомпрессия файла
     * \param input_file сжатый файл
     * \param output_file файл, в который сохраним данные
     * \param dictionary_file файл словаря для декомпресии
     * \return вернет 0 в случае успеха
     */
    int decompress_file(
            std::string input_file,
            std::string output_file,
            std::string dictionary_file) {
        int input_file_size = bf::get_file_size(input_file);
        if(input_file_size <= 0)
            return NOT_OPEN_FILE;

        int dictionary_file_size = bf::get_file_size(dictionary_file);
        if(dictionary_file_size <= 0)
            return NOT_OPEN_FILE;

        void *input_file_buffer = NULL;
        input_file_buffer = malloc(input_file_size);
        memset(input_file_buffer, 0, input_file_size);

        void *dictionary_file_buffer = NULL;
        dictionary_file_buffer = malloc(dictionary_file_size);
        memset(dictionary_file_buffer, 0, dictionary_file_size);

        bf::load_file(dictionary_file, dictionary_file_buffer, dictionary_file_size);
        bf::load_file(input_file, input_file_buffer, input_file_size);

        unsigned long long const decompress_file_size = ZSTD_getFrameContentSize(input_file_buffer, input_file_size);
        if (decompress_file_size == ZSTD_CONTENTSIZE_ERROR) {
            std::cout << input_file << " it was not compressed by zstd." << std::endl;
            free(dictionary_file_buffer);
            free(input_file_buffer);
            return NOT_DECOMPRESS_FILE;
        } else
        if (decompress_file_size == ZSTD_CONTENTSIZE_UNKNOWN) {
            std::cout << input_file << " original size unknown." << std::endl;
            free(dictionary_file_buffer);
            free(input_file_buffer);
            return NOT_DECOMPRESS_FILE;
        }
        void *decompress_file_buffer = NULL;
        decompress_file_buffer = malloc(decompress_file_size);
        memset(decompress_file_buffer, 0, decompress_file_size);

        ZSTD_DCtx* const dctx = ZSTD_createDCtx();
        size_t const decompress_size = ZSTD_decompress_usingDict(
            dctx,
            decompress_file_buffer,
            decompress_file_size,
            input_file_buffer,
            input_file_size,
            dictionary_file_buffer,
            dictionary_file_size);

        if(ZSTD_isError(decompress_size)) {
            std::cout << "error decompressin: " << ZSTD_getErrorName(decompress_size) << std::endl;
            ZSTD_freeDCtx(dctx);
            free(decompress_file_buffer);
            free(dictionary_file_buffer);
            free(input_file_buffer);
            return NOT_DECOMPRESS_FILE;
        }

        bf::write_file(output_file, decompress_file_buffer, decompress_size);

        ZSTD_freeDCtx(dctx);
        free(decompress_file_buffer);
        free(dictionary_file_buffer);
        free(input_file_buffer);
        return OK;
    }

    /** \brief Считать данные из сжатого файла
     * \param file_name имя файла
     * \param dictionary_file файл словаря для декомпресии
     * \param buffer буфер, в который запишем
     * \param buffer_size размер буфера
     * \return вернет 0 в случае успеха
     */
    int read_compressed_file(
            std::string file_name,
            std::string dictionary_file,
            void *&buffer,
            size_t &buffer_size) {
        int input_file_size = bf::get_file_size(file_name);
        if(input_file_size <= 0) return NOT_OPEN_FILE;

        int dictionary_file_size = bf::get_file_size(dictionary_file);
        if(dictionary_file_size <= 0) return NOT_OPEN_FILE;

        void *input_file_buffer = NULL;
        input_file_buffer = malloc(input_file_size);
        memset(input_file_buffer, 0, input_file_size);

        void *dictionary_file_buffer = NULL;
        dictionary_file_buffer = malloc(dictionary_file_size);
        memset(dictionary_file_buffer, 0, dictionary_file_size);

        bf::load_file(dictionary_file, dictionary_file_buffer, dictionary_file_size);
        bf::load_file(file_name, input_file_buffer, input_file_size);

        unsigned long long const decompress_file_size = ZSTD_getFrameContentSize(input_file_buffer, input_file_size);
        if(decompress_file_size == ZSTD_CONTENTSIZE_ERROR) {
            std::cout << file_name << " it was not compressed by zstd." << std::endl;
            free(dictionary_file_buffer);
            free(input_file_buffer);
            return NOT_DECOMPRESS_FILE;
        } else
        if(decompress_file_size == ZSTD_CONTENTSIZE_UNKNOWN) {
            std::cout << file_name << " original size unknown." << std::endl;
            free(dictionary_file_buffer);
            free(input_file_buffer);
            return NOT_DECOMPRESS_FILE;
        }

        buffer = malloc(decompress_file_size);
        memset(buffer,0,decompress_file_size); // решает проблему битой послденей временной метки

        ZSTD_DCtx* const dctx = ZSTD_createDCtx();
        size_t const decompress_size = ZSTD_decompress_usingDict(
            dctx,
            buffer,
            decompress_file_size,
            input_file_buffer,
            input_file_size,
            dictionary_file_buffer,
            dictionary_file_size);

        if(ZSTD_isError(decompress_size)) {
            std::cout << "error decompressin: " << ZSTD_getErrorName(decompress_size) << std::endl;
            ZSTD_freeDCtx(dctx);
            free(buffer);
            free(dictionary_file_buffer);
            free(input_file_buffer);
            buffer_size = 0;
            return NOT_DECOMPRESS_FILE;
        }
        buffer_size = decompress_size;
        ZSTD_freeDCtx(dctx);
        free(dictionary_file_buffer);
        free(input_file_buffer);
        return OK;
    }

    /** \brief Читать сжатый файл котировок
     * \param file_name имя файла
     * \param dictionary_file файл словаря для декомпресии
     * \param prices котировки
     * \param times временные метки
     * \return вернет 0 в случае успеха
     */
    int read_binary_quotes_compress_file(std::string file_name,
                                         std::string dictionary_file,
                                         std::vector<double> &prices,
                                         std::vector<unsigned long long> &times) {
        void *buffer = NULL;
        size_t buffer_size = 0;
        int err = read_compressed_file(file_name, dictionary_file, buffer, buffer_size);
        if(err == OK) {
            size_t buffer_offset = 0;
            unsigned long data_size = 0;
            std::memcpy(&data_size, (unsigned char*)buffer + buffer_offset, sizeof(data_size));
            buffer_offset += sizeof(data_size);
            prices.resize(data_size);
            times.resize(data_size);
            for(unsigned long i = 0; i < data_size; ++i) {
                std::memcpy(&prices[i], (unsigned char*)buffer + buffer_offset, sizeof(double));
                buffer_offset += sizeof(double);
                std::memcpy(&times[i], (unsigned char*)buffer + buffer_offset, sizeof(unsigned long long));
                buffer_offset += sizeof(unsigned long long);
            }
            if(buffer != NULL) {
                free(buffer);
            }
            return OK;
        } else {
            if(buffer != NULL) {
                free(buffer);
            }
            return err;
        }
    }
}

#endif // ZSTDEASY_HPP_INCLUDED
