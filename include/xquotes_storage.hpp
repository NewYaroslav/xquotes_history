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

/** \file Файл с классом хранилища данных
 * \brief Данный файл содержит класс Storage хранилища данных
 *
 * Класс Storage является родителем класса QuotesHistory и может быть
 * применен для других целей, например для хранения результатов оптимизации торговой стратегии
 */
#ifndef XQUOTES_STORAGE_HPP_INCLUDED
#define XQUOTES_STORAGE_HPP_INCLUDED

#include "xquotes_common.hpp"
#include "banana_filesystem.hpp"
#include "xtime.hpp"
#include <limits>
#include <algorithm>
#include <random>
#include <cstdio>
#include <memory>

#ifndef XQUOTES_NOT_USE_ZSTD
#define XQUOTES_USE_ZSTD 1
#include "dictBuilder/zdict.h"
#include "zstd.h"
#else
#define XQUOTES_USE_ZSTD 0
#endif

namespace xquotes_storage {
    using namespace xquotes_common;

    /** \brief Класс для работы с файлом-хранилищем котировок
     * \details Данный класс является родителем классов-хранилищ данных
     */
    class Storage {
        protected:
        std::fstream file;                              /**< Файл данных */
        std::string file_name;                          /**< Имя файла данных */
        bool is_write = false;                          /**< Флаг записи данных. Данный флаг устанавливается, если была хотя бы одна запись в файл*/
        bool is_file_open = false;                      /**< Фдаг наличия файла данных */
        char *dictionary_file_buffer = NULL;            /**< Указатель на буфер для хранения словаря */
        int dictionary_file_size = 0;
        bool is_mem_dict_file = false;                  /**< Флаг использования выделения памяти под словарь */
        note_t file_note = 0;                           /**< Заметка файла */

        std::unique_ptr<char[]> compressed_file_buffer;   /**< Буфер для записи */
        size_t compressed_file_buffer_size = 0;           /**< Размер буфера для записи */

        /** \brief Изменить размер буффера для записи сжатого файла
         * \details Данная функция работает только на увеличение размера буффера
         * \param new_size новый размер
         */
        void increase_compressed_file_buffer_size(const size_t new_size) {
            if(new_size > compressed_file_buffer_size) {
                compressed_file_buffer = std::unique_ptr<char[]>(new char[new_size]);
                compressed_file_buffer_size = new_size;
            }
        }

        std::unique_ptr<char[]> input_subfile_buffer;
        size_t input_subfile_buffer_size = 0;

        /** \brief Изменить размер буффера для чтения сжатого файла
         * \details Данная функция работает только на увеличение размера буффера
         * \param new_size новый размер
         */
        void increase_input_subfile_buffer_size(const size_t new_size) {
            if(new_size > input_subfile_buffer_size) {
                input_subfile_buffer = std::unique_ptr<char[]>(new char[new_size]);
                input_subfile_buffer_size = new_size;
            }
        }


        /** \brief Класс подфайла
         */
        class Subfile {
            public:
            key_t key = 0;          /**< Ключ подфайла */
            unsigned long size = 0; /**< Размер подфайла */
            link_t link = 0;        /**< Ссылка на подфайл */
            Subfile() {};

            Subfile(const key_t &key, const unsigned long &size, const link_t &link) {
                Subfile::key = key;
                Subfile::size = size;
                Subfile::link = link;
            };
        };

        std::vector<Subfile> subfiles;    /**< Подфайлы */

        Subfile *get_max_link(std::vector<Subfile> &_subfiles) {
            if(_subfiles.size() == 0) return NULL;
            if(_subfiles.size() == 1) return &_subfiles[0];
            auto subfiles_it = std::max_element(
                _subfiles.begin(),
                _subfiles.end(),
                [](const Subfile &lhs, const Subfile &rhs) {
                return lhs.link < rhs.link;
            });
            return &(*subfiles_it);
        }

        void sort_subfiles(std::vector<Subfile> &_subfiles) {
            if(!std::is_sorted(_subfiles.begin(), _subfiles.end(), [](const Subfile &a, const Subfile &b) {
                        return a.key < b.key;
                    })) {
                std::sort(_subfiles.begin(), _subfiles.end(), [](const Subfile &a, const Subfile &b) {
                    return a.key < b.key;
                });;
            }
        }

        Subfile *find_subfiles(const key_t key, std::vector<Subfile> &_subfiles) const {
            if(_subfiles.size() == 0) return NULL;
            auto subfiles_it = std::lower_bound(
                _subfiles.begin(),
                _subfiles.end(),
                key,
                [](const Subfile &lhs, const key_t &key) {
                return lhs.key < key;
            });
            if(subfiles_it == _subfiles.end()) {
                return NULL;
            } else
            if(subfiles_it->key == key) {
                return &(*subfiles_it);
            }
            return NULL;
        }

        void add_or_update_subfiles(const key_t key, const unsigned long size, const link_t link, std::vector<Subfile> &_subfiles) {
            if(_subfiles.size() == 0) {
                _subfiles.push_back(Subfile(key, size, link));
            } else {
                auto subfiles_it = std::upper_bound(
                    _subfiles.begin(),
                    _subfiles.end(),
                    key,
                    [](const key_t &key, const Subfile &rhs) {
                    return key < rhs.key;
                });
                if(subfiles_it == _subfiles.end() || subfiles_it == _subfiles.begin()) {
                     _subfiles.insert(subfiles_it, Subfile(key, size, link));
                } else
                if(subfiles_it->key == key) {
                    *subfiles_it = Subfile(key, size, link);
                }
            }
        }

        std::string get_random_name(const int &seed) {
            std::mt19937 gen;
            gen.seed(seed);
            static const char alphanum[] =
                "0123456789"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "abcdefghijklmnopqrstuvwxyz";
            std::uniform_int_distribution<> rnd_alphanum(0, sizeof(alphanum)-1);
            std::uniform_int_distribution<> rnd_length(5, 30);
            std::string temp = "";
            int length = rnd_length(gen);
            for(int i = 0; i < length; ++i) {
                temp += alphanum[rnd_alphanum(gen)];
            }
            temp += ".tmp";
            return temp;
        }

        inline void seek(const unsigned long offset, const std::ios::seekdir &origin, std::fstream &_file) {
            _file.clear();
            _file.seekg(offset, origin);
            _file.clear();
        }

        void read_header(std::fstream &_file, std::vector<Subfile> &_subfiles) {
            // прочитаем ссылку на заголовок
            seek(0, std::ios::beg, _file);
            unsigned long link_header = 0;
            _file.read(reinterpret_cast<char *>(&link_header), sizeof(link_header));
            // прочитаем количество подфайлов
            seek(link_header, std::ios::beg, _file);
            unsigned long num_subfiles = 0;
            _file.read(reinterpret_cast<char *>(&num_subfiles), sizeof(num_subfiles));
            if(num_subfiles == 0) {
                _subfiles.clear();
                return;
            }
            _subfiles.resize(num_subfiles);
            for(unsigned long i = 0; i < num_subfiles; ++i) {
                _file.read(reinterpret_cast<char *>(&_subfiles[i].key), sizeof(key_t));
                _file.read(reinterpret_cast<char *>(&_subfiles[i].size), sizeof(unsigned long));
                _file.read(reinterpret_cast<char *>(&_subfiles[i].link), sizeof(link_t));
            }
            _file.read(reinterpret_cast<char *>(&file_note), sizeof(file_note));
            sort_subfiles(_subfiles);
        }

        inline bool open(const std::string &path) {
            is_file_open = false;
            is_write = false; // сбрасываем флаг записи подфайлов
            file = std::fstream(path, std::ios_base::binary | std::ios::in | std::ios::out | std::ios::ate);
            if(!file.is_open()) return false;
            read_header(file, subfiles);
            is_file_open = true;
            return true;
        }

        void write_header(std::fstream &_file, std::vector<Subfile> &_subfiles) {
            if(_subfiles.size() == 0) return;
            sort_subfiles(_subfiles);
            Subfile *subfile_max_link = get_max_link(_subfiles);

            if(subfile_max_link == NULL) return;

            // запишем позицию ссылки на заголовок
            unsigned long link_header = subfile_max_link->link + subfile_max_link->size;

            seek(0, std::ios::beg, _file);
            _file.write(reinterpret_cast<char *>(&link_header), sizeof(link_header));
            // запишем кол-во файлов
            seek(link_header, std::ios::beg, _file);
            unsigned long num_subfiles = _subfiles.size();
            _file.write(reinterpret_cast<char *>(&num_subfiles), sizeof(num_subfiles));

            // пишем ссылки и ключи
            for(unsigned long i = 0; i < num_subfiles; ++i) {
                _file.write(reinterpret_cast<char *>(&_subfiles[i].key), sizeof(key_t));
                _file.write(reinterpret_cast<char *>(&_subfiles[i].size), sizeof(unsigned long));
                _file.write(reinterpret_cast<char *>(&_subfiles[i].link), sizeof(link_t));
            }
            _file.write(reinterpret_cast<char *>(&file_note), sizeof(file_note));
			_file.flush();
        }

        inline bool create_file(const std::string &file_name) {
            std::fstream file(file_name, std::ios::out | std::ios::app);
            if(!file) return false;
            file.close();
            return true;
        }

        // последний подфайл, который был найден
        unsigned long long last_key_found = 0;
        unsigned long last_link_found = 0;
        unsigned long last_size_found = 0;
        bool is_subfile_found = false;

        /** \brief Сохранить найденных подфайл (только его параметры!)
         */
        void save_subfile_found(const Subfile *subfile) {
            last_size_found = subfile->size;
            last_key_found = subfile->key;
            last_link_found = subfile->link;
            is_subfile_found = true;
        }

        int write_subfile_to_beg(const key_t key, const char *buffer, const unsigned long length) {
            seek(0, std::ios::beg, file);
            unsigned long temp = 0;
            file.write(reinterpret_cast<char *>(&temp), sizeof(temp));
            file.write(buffer, length);
            file.flush();
            add_or_update_subfiles(key, length, sizeof(unsigned long), subfiles);
            is_write = true;
            return OK;
        }

        int write_subfile_to_end(const key_t key, const char *buffer, const unsigned long length) {
            Subfile *subfile_max_link = get_max_link(subfiles);
            if(subfile_max_link == NULL) return INVALID_PARAMETER;
            unsigned long link = subfile_max_link->link + subfile_max_link->size;
            seek(link, std::ios::beg, file);
            file.write(buffer, length);
            file.flush();
            add_or_update_subfiles(key, length, link, subfiles);
            is_write = true;
            return OK;
        }

        int rewrite_subfile(const key_t key, const Subfile *subfile, const char *buffer, const unsigned long length) {
            if(subfile->size != length) {
                if(is_subfile_found && last_key_found == key) {
                    is_subfile_found = false;
                }
                // находим случайное имя файла
                int seed = xtime::get_millisecond();
                std::string temp_file = "";
                while(true) {
                    temp_file = get_random_name(seed);
                    if(!bf::check_file(temp_file)) break;
                    ++seed;
                }
                if(!create_file(temp_file)) return FILE_CANNOT_OPENED;

                /* создаем файл и пишем в него */
                std::fstream new_file = std::fstream(temp_file, std::ios_base::binary | std::ios::in | std::ios::out | std::ios::ate);
                if(!new_file) return FILE_CANNOT_OPENED;

                std::vector<Subfile> new_subfiles;
                unsigned long new_file_link = sizeof(unsigned long);
                for(size_t i = 0; i < subfiles.size(); ++i) {
                    if(subfiles[i].key != subfile->key) {
                        // если это не переписываемый файл, просто копируем его
                        seek(subfiles[i].link, std::ios::beg, file);
                        char *copy_buffer = new char[subfiles[i].size];
                        file.read(copy_buffer, subfiles[i].size);
                        if(i == 0) {
                            unsigned long temp = 0;
                            new_file.write(reinterpret_cast<char *>(&temp), sizeof(unsigned long));
                            new_file.write(copy_buffer, subfiles[i].size);
                            add_or_update_subfiles(subfiles[i].key, subfiles[i].size, new_file_link, new_subfiles);
                        } else {
                            new_file.write(copy_buffer, subfiles[i].size);
                            add_or_update_subfiles(subfiles[i].key, subfiles[i].size, new_file_link, new_subfiles);
                        }
                        delete [] copy_buffer;
                        new_file_link += subfiles[i].size;
                    } else {
                        // если это переписываемый файл, пишем новые данные в новый файл
                        if(i == 0) {
                            unsigned long temp = 0;
                            new_file.write(reinterpret_cast<char *>(&temp), sizeof(unsigned long));
                            new_file.write(buffer, length);
                            add_or_update_subfiles(subfile->key, length, new_file_link, new_subfiles);
                        } else {
                            new_file.write(buffer, length);
                            add_or_update_subfiles(subfile->key, length, new_file_link, new_subfiles);
                        }
                        new_file_link += length;
                    }
                }

                /* запишем заголовок в новый файл */
                write_header(new_file, new_subfiles);

                /* закромем все файлы и обнулим флаг открытия файла */
                new_file.close();
                file.close();
                is_file_open = false;

                int err = 0;
                if((err = remove(file_name.c_str())) != 0) {
                    //std::cout << "xquotes storage error, what: remove(" << file_name << "), code: " << err << std::endl;
                    return FILE_CANNOT_REMOVED;
                }
                if(rename(temp_file.c_str(), file_name.c_str()) != 0) return FILE_CANNOT_RENAMED;
                if(!open(file_name)) return FILE_CANNOT_OPENED;
            } else {
                seek(subfile->link, std::ios::beg, file);
                file.write(buffer, subfile->size);
            }
            return OK;
        }

        /* добавляем рассчет crc64 */

        const long long poly = 0xC96C5795D7870F42;
        long long crc64_table[256];

        void crc64_generate_table() {
            for(uint32_t i = 0; i < 256; ++i) {
                long long crc = i;
                for(uint32_t j = 0; j < 8; ++j) {
                    if(crc & 1) {
                        crc >>= 1;
                        crc ^= poly;
                    } else {
                        crc >>= 1;
                    }
                }
                crc64_table[i] = crc;
            }
        }

        long long calculate_crc64(long long crc, const unsigned char* stream, uint32_t n) {
            for(uint32_t i = 0; i < n; ++i) {
                unsigned char index = stream[i] ^ crc;
                long long lookup = crc64_table[index];
                crc >>= 8;
                crc ^= lookup;
            }
            return crc;
        }

        public:

        Storage() {
            crc64_generate_table();
        };

        /** \brief Инициализировать класс хранилища
         * \param path путь к файлу с данными
         * \param dictionary_file путь к файлу словаря для архивирования данных. По умолчанию не используется
         */
        Storage(const std::string &path, const std::string &dictionary_file = "") {
            crc64_generate_table();
            file_name = path;
            if(!bf::check_file(path)) {
                if(!create_file(path)) return;
            }
            open(path);
            if(dictionary_file != "" && bf::check_file(dictionary_file)) {
                dictionary_file_size = bf::get_file_size(dictionary_file);
                if(dictionary_file_size <= 0) {
                    is_file_open = false;
                    return;
                }
                dictionary_file_buffer = new char[dictionary_file_size];
                std::fill(dictionary_file_buffer, dictionary_file_buffer + dictionary_file_size, '\0');
                bf::load_file(dictionary_file, dictionary_file_buffer, dictionary_file_size);
                is_mem_dict_file = true; // ставим флаг использования памяти под словарь
            }
        }

        /** \brief Инициализировать класс хранилища
         * \param path путь к файлу с данными
         * \param dictionary_buffer указатель на буфер словаря
         * \param dictionary_buffer_size размер буфера словаря
         */
        Storage(const std::string &path, const char *dictionary_buffer, const size_t dictionary_buffer_size) {
            crc64_generate_table();
            file_name = path;
            if(!bf::check_file(path)) {
                if(!create_file(path)) return;
            }
            open(path);
            dictionary_file_buffer = (char*)dictionary_buffer;
            dictionary_file_size = dictionary_buffer_size;
        }

        /** \brief Инициализировать класс хранилища
         * \param path путь к файлу с данными
         * \param dictionary_buffer указатель на буфер словаря
         * \param dictionary_buffer_size размер буфера словаря
         * \return вернет true, если инициализация прошла успешно
         */
        bool init(const std::string &path, const char *dictionary_buffer, const size_t dictionary_buffer_size) {
            if(is_file_open) return false;
            file_name = path;
            if(!bf::check_file(path)) {
                if(!create_file(path)) return false;
            }
            if(!open(path)) return false;
            dictionary_file_buffer = (char*)dictionary_buffer;
            dictionary_file_size = dictionary_buffer_size;
            return true;
        }

        /** \brief Инициализировать класс хранилища
         * \param path путь к файлу с данными
         * \return вернет true, если инициализация прошла успешно
         */
        bool init(const std::string &path) {
            if(is_file_open) return false;
            file_name = path;
            if(!bf::check_file(path)) {
                if(!create_file(path)) return false;
            }
            if(!open(path)) return false;
            return true;
        }

        /** \brief Инициализировать указатель на словарь
         * \param dictionary_buffer указатель на буфер словаря
         * \param dictionary_buffer_size размер буфера словаря
         */
        void set_dictionary(const char *dictionary_buffer, const size_t dictionary_buffer_size) {
            dictionary_file_buffer = (char*)dictionary_buffer;
            dictionary_file_size = dictionary_buffer_size;
        }

        /** \brief Получить размер подфайла
         * \details Данная функция позволяет узнать размер подфайла
         * \param key ключ подфайла
         * \param size размер подфайла
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int get_subfile_size(const key_t key, unsigned long &size) {
            if(subfiles.size() == 0) return NO_SUBFILES;
            if(is_subfile_found && last_key_found == key) {
                size = last_size_found;
                return OK;
            }
            Subfile *subfile = find_subfiles(key, subfiles);
            if(subfile == NULL) return SUBFILES_NOT_FOUND;
            size = subfile->size;
            // запоминаем найденный файл
            save_subfile_found(subfile);
            return OK;
        }

        /** \brief Прочитать подфайл
         * \warning После каждого вызова данной функции необходимо очистить буфер! (если он не был инициализирован заранее)
         * \param key ключ подфайла
         * \param buffer буфер для чтения файла. Если равен NULL, функция сама выделит память!
         * \param buffer_size сюда будет помещен размер буфера (подфайла)
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int read_subfile(const key_t key, char *&buffer, unsigned long &buffer_size) {
            if(!is_file_open) return FILE_NOT_OPENED;
            // если ранее мы уже нашли подфайл
            if(is_subfile_found && last_key_found == key) {
                seek(last_link_found, std::ios::beg, file);
                if(buffer == NULL) buffer = new char[last_size_found];
                file.read(buffer, last_size_found);
				buffer_size = last_size_found;
                return OK;
            }

            if(subfiles.size() == 0) return NO_SUBFILES;
            Subfile *subfile = find_subfiles(key, subfiles);
            if(subfile == NULL) return SUBFILES_NOT_FOUND;

            // запоминаем найденный подфайл
            save_subfile_found(subfile);
            buffer_size = last_size_found;
            seek(subfile->link, std::ios::beg, file);
            if(buffer == NULL) buffer = new char[last_size_found];
            file.read(buffer, subfile->size);
            return OK;
        }

        /** \brief Прочитать подфайл
         * \warning Данная функция может сама инициализировать буфер
         * \param key ключ подфайла
         * \param read_buffer буфер для чтения файла
         * \param buffer read_buffer_size размер буфера для чтения, внутри метода может только увеличиться!
         * \param buffer_size сюда будет помещен размер подфайла
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int read_subfile(
                const key_t key,
                std::unique_ptr<char[]> &read_buffer,
                size_t &read_buffer_size,
                unsigned long &buffer_size) {
            if(!is_file_open) return FILE_NOT_OPENED;
            // если ранее мы уже нашли подфайл
            if(is_subfile_found && last_key_found == key) {
                seek(last_link_found, std::ios::beg, file);
                if(last_size_found > read_buffer_size) {
                    read_buffer = std::unique_ptr<char[]>(new char[last_size_found]);
                    read_buffer_size = last_size_found;
                }
                file.read(read_buffer.get(), last_size_found);
				buffer_size = last_size_found;
                return OK;
            }

            if(subfiles.size() == 0) return NO_SUBFILES;
            Subfile *subfile = find_subfiles(key, subfiles);
            if(subfile == NULL) return SUBFILES_NOT_FOUND;

            // запоминаем найденный подфайл
            save_subfile_found(subfile);
            seek(subfile->link, std::ios::beg, file);

            if(last_size_found > read_buffer_size) {
                read_buffer = std::unique_ptr<char[]>(new char[last_size_found]);
                read_buffer_size = last_size_found;
            }
            file.read(read_buffer.get(), subfile->size);
			buffer_size = last_size_found;
            return OK;
        }

        /** \brief Записать подфайл
         * \warning Если требуется перезаписать старый подфайл, размер которого изменился,
         * данный метод может создать временный файл для копирования данных!
         * \param key ключ подфайла
         * \param buffer буфер для записи файла
         * \param buffer_size размер буфера (размер подфайла)
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int write_subfile(const key_t key, const char *buffer, const unsigned long &buffer_size) {
            if(!is_file_open) return FILE_NOT_OPENED;
            if(subfiles.size() == 0) {
                return write_subfile_to_beg(key, buffer, buffer_size);
            }
            Subfile *subfile = find_subfiles(key, subfiles);
            if(subfile == NULL) {
                return write_subfile_to_end(key, buffer, buffer_size);
            } else {
                return rewrite_subfile(key, subfile, buffer, buffer_size);
            }
            return OK;
        }

        /** \brief Проверить наличие файла
         * \param key ключ подфайла
         * \return вернет true если файл найден
         */
        bool check_subfile(const key_t key) {
            if(subfiles.size() == 0) return false;
            if(is_subfile_found && last_key_found == key) return true;
            const Subfile *subfile = find_subfiles(key, subfiles);
            if(subfile == NULL) return false;
            save_subfile_found(subfile);
            return true;
        }

        /** \brief Получить список подфайлов, начинаюбщихся с определенного ключа
         * \warning Файл с ключем key тоже будет в списке list_subfile!
         * \param key ключ подфайла, с кооторого будет начат поиск
         * \param list_subfile список файлов
         * \param f функтор для условия пропуска ключей подфайлов
         * \param is_go_to_beg идти к началу списка или к концу, если false
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int get_subfile_list(
                const key_t key,
                std::vector<key_t>& list_subfile,
                const int &num_subfile,
                bool (*f)(const key_t &key) = NULL,
                const bool &is_go_to_beg = true) {
            if(subfiles.size() == 0) return DATA_NOT_AVAILABLE;
            auto subfiles_it = std::lower_bound(
                subfiles.begin(),
                subfiles.end(),
                key,
                [](const Subfile &lhs, const key_t &key) {
                return lhs.key < key;
            });
            if(subfiles_it == subfiles.end()) {
                return DATA_NOT_AVAILABLE;
            }
            int ind = (int)(subfiles_it - subfiles.begin());
            list_subfile.clear();
            if(is_go_to_beg) {
                if(ind > 0 && subfiles[ind].key > key) {
                    --ind;
                } else
                if(ind == 0 && subfiles[ind].key > key) return DATA_NOT_AVAILABLE;
                while(ind >= 0 && (int)list_subfile.size() < num_subfile) {
                    if(f != NULL && f(subfiles[ind].key)) {
                        --ind;
                        continue;
                    }
                    list_subfile.push_back(subfiles[ind].key);
                    --ind;
                }
                std::reverse(list_subfile.begin(), list_subfile.end());
            } else {
                if(ind < ((int)subfiles.size() + 1) && subfiles[ind].key < key) {
                    ++ind;
                } else
                if(ind == ((int)subfiles.size() + 1) && subfiles[ind].key < key) return DATA_NOT_AVAILABLE;
                while(ind < (int)subfiles.size() && (int)list_subfile.size() < num_subfile) {
                    if(f != NULL && f(subfiles[ind].key)) {
                        ++ind;
                        continue;
                    }
                    list_subfile.push_back(subfiles[ind].key);
                    ++ind;
                }
            }
            return OK;
        }

#if     XQUOTES_USE_ZSTD == 1

        /** \brief Записать сжатый подфайл
         * \warning Данная функция для декомпрессии использует словарь!
         * \param key ключ подфайла
         * \param buffer буфер для записи файла
         * \param buffer_size размер буфера (размер подфайла до компрессии)
         * \param compress_level уровень сжатия, по умолчанию максимальный
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int write_compressed_subfile(
                const key_t key,
                const char *buffer,
                const unsigned long &buffer_size,
                int compress_level = ZSTD_maxCLevel()) {
            if(!is_file_open) return FILE_NOT_OPENED;
            size_t new_compressed_file_size = ZSTD_compressBound(buffer_size);
            increase_compressed_file_buffer_size(new_compressed_file_size);

            //char *compressed_file_buffer = new char[compressed_file_size];
            std::fill(compressed_file_buffer.get(), compressed_file_buffer.get() + new_compressed_file_size, '\0');

            ZSTD_CCtx* const cctx = ZSTD_createCCtx();
            size_t compressed_size = ZSTD_compress_usingDict(
                cctx,
                compressed_file_buffer.get(),
                new_compressed_file_size,
                buffer,
                buffer_size,
                dictionary_file_buffer,
                dictionary_file_size,
                compress_level
                );

            if(ZSTD_isError(compressed_size)) {
                //std::cout << "compression error: " << ZSTD_getErrorName(compress_size) << std::endl;
                ZSTD_freeCCtx(cctx);
                //delete [] compressed_file_buffer;
                return SUBFILES_COMPRESSION_ERROR;
            }
            int err = write_subfile(key, compressed_file_buffer.get(), compressed_size);
            ZSTD_freeCCtx(cctx);
            //delete [] compressed_file_buffer;
            return err;
        }

        /** \brief Прочитать сжатый подфайл
         * \warning После каждого вызова данной функции необходимо очистить буфер!
         * Данная функция для декомпрессии использует словарь!
         * \param key ключ подфайла
         * \param buffer буфер для чтения файла. Если равен NULL, функция сама выделит память!
         * \param buffer_size сюда будет помещен размер подфайла после декомпресси
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int read_compressed_subfile(const key_t key, char *&buffer, unsigned long& buffer_size) {
            if(!is_file_open) return FILE_NOT_OPENED;
            if(subfiles.size() == 0) return NO_SUBFILES;
            //char *input_subfile_buffer = NULL;
            // если ранее мы уже нашли подфайл
            if(is_subfile_found && last_key_found == key) {
                increase_input_subfile_buffer_size(last_size_found);
                seek(last_link_found, std::ios::beg, file);
                file.read(input_subfile_buffer.get(), last_size_found);
            } else {
                Subfile *subfile = find_subfiles(key, subfiles);
                if(subfile == NULL) return SUBFILES_NOT_FOUND;
                increase_input_subfile_buffer_size(subfile->size);
                seek(subfile->link, std::ios::beg, file);
                file.read(input_subfile_buffer.get(), subfile->size);
                save_subfile_found(subfile);
            }

            const unsigned long long decompress_file_size = ZSTD_getFrameContentSize(input_subfile_buffer.get(), last_size_found);
            if(decompress_file_size == ZSTD_CONTENTSIZE_ERROR) {
                return NOT_DECOMPRESS_FILE;
            } else
            if(decompress_file_size == ZSTD_CONTENTSIZE_UNKNOWN) {
                return NOT_DECOMPRESS_FILE;
            }

            bool is_init_buffer = false;
            if(buffer == NULL) {
                buffer = new char [decompress_file_size];
                is_init_buffer = true;
            }
            std::fill(buffer, buffer + decompress_file_size, '\0');

            // на данном этапе переменная last_size_found в любом случае будет содержать размер файла
            ZSTD_DCtx* const dctx = ZSTD_createDCtx();
            const size_t subfile_size = ZSTD_decompress_usingDict(
                dctx,
                buffer,
                decompress_file_size,
                input_subfile_buffer.get(),
                last_size_found,
                dictionary_file_buffer,
                dictionary_file_size);

            if(ZSTD_isError(subfile_size)) {
                //std::cout << "error decompressin: " << ZSTD_getErrorName(subfile_size) << std::endl;
                ZSTD_freeDCtx(dctx);
                if(is_init_buffer) {
                    delete [] buffer;
                    buffer =  NULL;
                }
                buffer_size = 0;
                return NOT_DECOMPRESS_FILE;
            }
            buffer_size = subfile_size;
            ZSTD_freeDCtx(dctx);
            return OK;
        }

        /** \brief Прочитать сжатый подфайл
         * \warning После каждого вызова данной функции необходимо очистить буфер!
         * Данная функция для декомпрессии использует словарь!
         * \param key ключ подфайла
         * \param read_buffer буфер для чтения файла
         * \param buffer read_buffer_size размер буфера для чтения, внутри метода может только увеличиться!
         * \param buffer_size сюда будет помещен размер подфайла после декомпресси
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int read_compressed_subfile(
                const key_t key,
                std::unique_ptr<char[]> &read_buffer,
                size_t &read_buffer_size,
                unsigned long& buffer_size) {
            if(!is_file_open) return FILE_NOT_OPENED;
            if(subfiles.size() == 0) return NO_SUBFILES;
            //char *input_subfile_buffer = NULL;
            // если ранее мы уже нашли подфайл
            if(is_subfile_found && last_key_found == key) {
                increase_input_subfile_buffer_size(last_size_found);
                seek(last_link_found, std::ios::beg, file);
                file.read(input_subfile_buffer.get(), last_size_found);
            } else {
                Subfile *subfile = find_subfiles(key, subfiles);
                if(subfile == NULL) return SUBFILES_NOT_FOUND;
                increase_input_subfile_buffer_size(subfile->size);
                seek(subfile->link, std::ios::beg, file);
                file.read(input_subfile_buffer.get(), subfile->size);
                save_subfile_found(subfile);
            }

            const unsigned long long decompress_file_size = ZSTD_getFrameContentSize(input_subfile_buffer.get(), last_size_found);
            if(decompress_file_size == ZSTD_CONTENTSIZE_ERROR) {
                return NOT_DECOMPRESS_FILE;
            } else
            if(decompress_file_size == ZSTD_CONTENTSIZE_UNKNOWN) {
                return NOT_DECOMPRESS_FILE;
            }

            if(decompress_file_size > read_buffer_size) {
                read_buffer = std::unique_ptr<char[]>(new char[decompress_file_size]);
                read_buffer_size = decompress_file_size;
            }
            char *buffer = read_buffer.get();
            std::fill(buffer, buffer + decompress_file_size, '\0');

            // на данном этапе переменная last_size_found в любом случае будет содержать размер файла
            ZSTD_DCtx* const dctx = ZSTD_createDCtx();
            const size_t subfile_size = ZSTD_decompress_usingDict(
                dctx,
                buffer,
                decompress_file_size,
                input_subfile_buffer.get(),
                last_size_found,
                dictionary_file_buffer,
                dictionary_file_size);

            if(ZSTD_isError(subfile_size)) {
                //std::cout << "error decompressin: " << ZSTD_getErrorName(subfile_size) << std::endl;
                ZSTD_freeDCtx(dctx);
                buffer_size = 0;
                return NOT_DECOMPRESS_FILE;
            }
            buffer_size = subfile_size;
            ZSTD_freeDCtx(dctx);
            return OK;
        }
#       endif // XQUOTES_USE_ZSTD

        /** \brief Сохранить файл хранилища
         */
        void save() {
            if(file.is_open()) {
                if(is_write) write_header(file, subfiles);
            }
        }

        /** \brief Закрыть файл хранилища
         */
        virtual void close() {
            if(file.is_open()) {
                if(is_write) write_header(file, subfiles);
                file.close();
            }
        }

        /** \brief Получить минимальный и максимальный ключ подфайлов
         * \param min_key минимальный ключ подфайлов
         * \param max_key максимальный ключ подфайлов
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int get_min_max_key(key_t &min_key, key_t &max_key) const {
            if(subfiles.size() == 0) return DATA_NOT_AVAILABLE;
            auto subfiles_max_it = std::max_element(
                subfiles.begin(),
                subfiles.end(),
                [](const Subfile &lhs, const Subfile &rhs) {
                return lhs.key < rhs.key;
            });
            auto subfiles_min_it = std::min_element(
                subfiles.begin(),
                subfiles.end(),
                [](const Subfile &lhs, const Subfile &rhs) {
                return lhs.key < rhs.key;
            });
            max_key = subfiles_max_it->key;
            min_key = subfiles_min_it->key;
            return OK;
        }

        /** \brief Получить количество подфайлов
         * \return количество подфайлов
         */
        inline size_t get_num_subfiles() {
            return subfiles.size();
        };

        /** \brief Получить ключ подфайла
         * \param ind индекс подфайла
         * \return ключ подфайла
         */
        inline key_t get_key_subfiles(const int ind) const {
            return subfiles[ind].key;
        }

        /** \brief Получить заметку файла
         * \return заметка файла (число, которое может хранить пользовательские биты настроек)
         */
        inline note_t get_file_note() const {
            return file_note;
        };

        /** \brief Установить заметку файла
         * \param new_file_note заметка файла (число, которое может хранить пользовательские биты настроек)
         */
        void set_file_note(note_t new_file_note) {file_note = new_file_note;};

        /** \brief Переименовать подфайл
         * \param key старый ключ подфайла
         * \param new_key новый ключ подфайла
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int rename_subfile(const key_t key, const key_t new_key) {
            if(subfiles.size() == 0) return DATA_NOT_AVAILABLE;
            auto subfiles_it = std::lower_bound(
                subfiles.begin(),
                subfiles.end(),
                key,
                [](const Subfile &lhs, const key_t &key) {
                return lhs.key < key;
            });
            if(subfiles_it == subfiles.end()) return DATA_NOT_AVAILABLE;
            size_t ind = (size_t)(subfiles_it - subfiles.begin());
            subfiles[ind].key = new_key;
            sort_subfiles(subfiles);
            return OK;
        }

        /** \brief Удалить подфайл
         * \param key старый ключ подфайла
         * \return вернет 0 в случае успеха, иначе см. код ошибок в xquotes_common.hpp
         */
        int delete_subfile(const key_t key) {
            if(subfiles.size() == 0) return DATA_NOT_AVAILABLE;
            auto subfiles_it = std::lower_bound(
                subfiles.begin(),
                subfiles.end(),
                key,
                [](const Subfile &lhs, const key_t &key) {
                return lhs.key < key;
            });
            if(subfiles_it == subfiles.end()) return DATA_NOT_AVAILABLE;
            subfiles.erase(subfiles_it);
            sort_subfiles(subfiles);
            write_header(file, subfiles);
            return OK;
        }

        /** \brief Получить crc64 код подфайла
         *
         * \param key Ключ подфайла
         * \return crc64 код
         */
        long long get_crc64_subfile(const key_t key) {
            std::unique_ptr<char[]> read_buffer;
            size_t read_buffer_size = 0;
            unsigned long buffer_size = 0;
            int err = read_subfile(key,
                read_buffer,
                read_buffer_size,
                buffer_size);
            if(err != OK || buffer_size == 0) return 0;
            const char *buffer = read_buffer.get();
            return calculate_crc64(0, (const unsigned char*)buffer, buffer_size);
        }

        virtual ~Storage() {
            close();
            if(is_mem_dict_file) delete [] dictionary_file_buffer;
        }
    };
}

#endif // XQUOTES_STORAGE_HPP_INCLUDED
