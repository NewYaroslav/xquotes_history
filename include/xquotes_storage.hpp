#ifndef XQUOTES_STORAGE_HPP_INCLUDED
#define XQUOTES_STORAGE_HPP_INCLUDED

#include "xquotes_common.hpp"
#include "banana_filesystem.hpp"
#include "xtime.hpp"
#include <limits>
#include <algorithm>
#include <random>
#include <cstdio>

#ifndef XQUOTES_NOT_USE_ZSTD
#define XQUOTES_USE_ZSTD 1
#include "dictBuilder/zdict.h"
#include "zstd.h"
#else
#define XQUOTES_USE_ZSTD 0
#endif

namespace xquotes_storage {
    using namespace xquotes_common;
//------------------------------------------------------------------------------
// РАБОТА С ФАЙЛОМ-ХРАНИЛИЩЕМ
//------------------------------------------------------------------------------
    class Storage {
        private:
        std::fstream file;                              /**< Файл данных */
        std::string file_name;
        bool is_write = false;
        bool is_file_open = false;
        char *dictionary_file_buffer = NULL;
        int dictionary_file_size = 0;
        /** \brief Класс подфайла
         */
        class Subfile {
            public:
            key_t key = 0;     /**< Ключ подфайла */
            unsigned long size = 0;         /**< Размер подфайла */
            key_t link = 0;         /**< Ссылка на подфайл */
            Subfile() {};

            Subfile(const key_t key, const unsigned long size, const link_t link) {
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

        Subfile *find_subfiles(const key_t key, std::vector<Subfile> &_subfiles) {
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
                if(subfiles_it == _subfiles.end()) {
                     _subfiles.insert(subfiles_it, Subfile(key, size, link));
                } else
                if(subfiles_it->key == key) {
                    *subfiles_it = Subfile(key, size, link);
                }
            }
        }

        std::string get_random_name(const int seed) {
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

        inline void seek(const unsigned long offset, const std::ios::seekdir origin, std::fstream &_file) {
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
            sort_subfiles(_subfiles);
        }

        inline bool open(std::string path) {
            is_file_open = false;
            is_write = false;
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
        }

        inline bool create_file(std::string file_name) {
            std::fstream file(file_name, std::ios::out | std::ios::app);
            if(!file) return false;
            file.close();
            return true;
        }

        unsigned long long last_key_found = 0;
        unsigned long last_link_found = 0;
        unsigned long last_size_found = 0;
        bool is_subfile_found = false;

        void save_subfile_found(const Subfile *subfile) {
            is_subfile_found = true;
            last_size_found = subfile->size;
            last_key_found = subfile->key;
            last_link_found = subfile->link;
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
                int seed = 0;
                std::string temp_file = "";
                while(true) {
                    temp_file = get_random_name(seed);
                    if(!bf::check_file(temp_file)) break;
                    seed++;
                }
                if(!create_file(temp_file)) return FILE_CANNOT_OPENED;
                // создаем файл и пишем в него
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
                        delete copy_buffer;
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
                write_header(new_file, new_subfiles);
                new_file.close();
                file.close();
                is_file_open = false;

                if(remove(file_name.c_str()) != 0) return FILE_CANNOT_REMOVED;
                if(rename(temp_file.c_str(), file_name.c_str()) != 0) return FILE_CANNOT_RENAMED;
                if(!open(file_name)) return FILE_CANNOT_OPENED;
            } else {
                seek(subfile->link, std::ios::beg, file);
                file.write(buffer, subfile->size);
            }
            return OK;
        }

        public:

        Storage() {};

        /** \brief Инициализировать класс хранилища
         * Данная функция позволяет узнать размер подфайла
         * \param path путь к файлу с данными
         * \param dictionary_file путь к файлу словаря для архивирования данных. По умолчанию не используется
         */
        Storage(std::string path, std::string dictionary_file = "") {
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
            }
        }

        /** \brief Получить размер подфайла
         * Данная функция позволяет узнать размер подфайла
         * \param key ключ подфайла
         * \param size размер подфайла
         * \return вернет 0 в случае отсутствия ошибок
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
         * \warning После каждого вызова данной функции необходимо очистить буфер!
         * \param key ключ подфайла
         * \param buffer буфер для чтения файла. Если равен NULL, функция сама выделит память!
         * \param buffer_size сюда будет помещен размер буфера (подфайла)
         * \return вернет 0 в случае отсутствия ошибок
         */
        int read_subfile(const key_t key, char *&buffer, unsigned long &buffer_size) {
            if(!is_file_open) return FILE_NOT_OPENED;
            // если ранее мы уже нашли подфайл
            if(is_subfile_found && last_key_found == key) {
                seek(last_link_found, std::ios::beg, file);
                if(buffer == NULL) buffer = new char[last_size_found];
                file.read(buffer, last_size_found);
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

        /** \brief Записать подфайл
         * \warning Если требуется перезаписать старый подфайл, размер которого изменился,
         * данный метод может создать временный файл для копирования данных!
         * \param key ключ подфайла
         * \param buffer буфер для записи файла
         * \param buffer_size размер буфера (размер подфайла)
         * \return вернет 0 в случае отсутствия ошибок
         */
        int write_subfile(const key_t key, const char *buffer, const unsigned long buffer_size) {
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
            Subfile *subfile = find_subfiles(key, subfiles);
            if(subfile == NULL) return false;
            save_subfile_found(subfile);
            return true;
        }

#if     XQUOTES_USE_ZSTD == 1

        /** \brief Записать сжатый подфайл
         * \warning Данная функция для декомпрессии использует словарь!
         * \param key ключ подфайла
         * \param buffer буфер для записи файла
         * \param buffer_size размер буфера (размер подфайла до компрессии)
         * \param compress_level уровень сжатия, по умолчанию максимальный
         * \return вернет 0 в случае отсутствия ошибок
         */
        int write_compressed_subfile(
                const key_t key,
                const char *buffer,
                const unsigned long buffer_size,
                int compress_level = ZSTD_maxCLevel()) {
            if(!is_file_open) return FILE_NOT_OPENED;
            size_t compressed_file_size = ZSTD_compressBound(buffer_size);
            char *compressed_file_buffer = new char[compressed_file_size];
            std::fill(compressed_file_buffer, compressed_file_buffer + compressed_file_size, '\0');

            ZSTD_CCtx* const cctx = ZSTD_createCCtx();
            size_t compressed_size = ZSTD_compress_usingDict(
                cctx,
                compressed_file_buffer,
                compressed_file_size,
                buffer,
                buffer_size,
                dictionary_file_buffer,
                dictionary_file_size,
                compress_level
                );

            if(ZSTD_isError(compressed_size)) {
                //std::cout << "compression error: " << ZSTD_getErrorName(compress_size) << std::endl;
                ZSTD_freeCCtx(cctx);
                delete compressed_file_buffer;
                return SUBFILES_COMPRESSION_ERROR;
            }
            int err = write_subfile(key, compressed_file_buffer, compressed_size);
            ZSTD_freeCCtx(cctx);
            delete compressed_file_buffer;
            return err;
        }

        /** \brief Прочитать сжатый подфайл
         * \warning После каждого вызова данной функции необходимо очистить буфер!
         * Данная функция для декомпрессии использует словарь!
         * \param key ключ подфайла
         * \param buffer буфер для чтения файла. Если равен NULL, функция сама выделит память!
         * \param buffer_size сюда будет помещен размер подфайла после декомпресси
         * \return вернет 0 в случае отсутствия ошибок
         */
        int read_compressed_subfile(const key_t key, char *&buffer, unsigned long& buffer_size) {
            if(!is_file_open) return FILE_NOT_OPENED;
            if(subfiles.size() == 0) return NO_SUBFILES;
            char *input_subfile_buffer = NULL;
            // если ранее мы уже нашли подфайл
            if(is_subfile_found && last_key_found == key) {
                input_subfile_buffer = new char[last_size_found];
                seek(last_link_found, std::ios::beg, file);
                file.read(input_subfile_buffer, last_size_found);
            } else {
                Subfile *subfile = find_subfiles(key, subfiles);
                if(subfile == NULL) return SUBFILES_NOT_FOUND;
                input_subfile_buffer = new char[subfile->size];
                seek(subfile->link, std::ios::beg, file);
                file.read(input_subfile_buffer, subfile->size);
                save_subfile_found(subfile);
            }

            const unsigned long long decompress_file_size = ZSTD_getFrameContentSize(input_subfile_buffer, last_size_found);
            if(decompress_file_size == ZSTD_CONTENTSIZE_ERROR) {
                std::cout << file_name << " it was not compressed by zstd." << std::endl;
                delete input_subfile_buffer;
                return NOT_DECOMPRESS_FILE;
            } else
            if(decompress_file_size == ZSTD_CONTENTSIZE_UNKNOWN) {
                std::cout << file_name << " original size unknown." << std::endl;
                delete input_subfile_buffer;
                return NOT_DECOMPRESS_FILE;
            }

            if(buffer == NULL) buffer = new char [decompress_file_size];
            std::fill(buffer, buffer + decompress_file_size, '\0');

            // на данном этапе переменная last_size_found в любом случае будет содержать размер файла
            const ZSTD_DCtx* dctx = ZSTD_createDCtx();
            const size_t subfile_size = ZSTD_decompress_usingDict(
                dctx,
                buffer,
                decompress_file_size,
                input_subfile_buffer,
                last_size_found,
                dictionary_file_buffer,
                dictionary_file_size);

            if(ZSTD_isError(subfile_size)) {
                //std::cout << "error decompressin: " << ZSTD_getErrorName(subfile_size) << std::endl;
                ZSTD_freeDCtx(dctx);
                delete buffer;
                delete input_subfile_buffer;
                buffer_size = 0;
                return NOT_DECOMPRESS_FILE;
            }
            buffer_size = subfile_size;
            ZSTD_freeDCtx(dctx);
            delete input_subfile_buffer;
            return OK;
        }
#       endif // XQUOTES_USE_ZSTD

        /** \brief Закрыть файл хранилища
         */
        void close() {
            if(file.is_open()) {
                if(is_write) write_header(file, subfiles);
                file.close();
            }
        }

        ~Storage() {
            close();
            delete dictionary_file_buffer;
        }
    };
}

#endif // XQUOTES_STORAGE_HPP_INCLUDED