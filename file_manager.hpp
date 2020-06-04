#ifndef __FILE_MANAGER_HPP__
#define __FILE_MANAGER_HPP__
/**************************************************************************
 *
 *     This file is part of Chiton.
 *
 *   Chiton is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Chiton is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Chiton.  If not, see <https://www.gnu.org/licenses/>.
 *
 *   Copyright 2020 Ed Martin <edman007@edman007.com>
 *
 **************************************************************************
 */

#include <string>
#include "database.hpp"
#include "chiton_config.hpp"

class FileManager {
public:
    FileManager(Database &db, Config &cfg) : db(db), cfg(cfg) {};

    //returns a valid path starting at start_time, writes out it's ID to int &file_id
    std::string get_next_path(long int &file_id, int camera, struct timeval &start_time);

    //returns the path for the file referenced as id
    std::string get_path(long int file_id);

    //update metadata about the file
    bool update_file_metadata(long int file_id, struct timeval &end_time);

    void clean_disk(void);//clean up the disk by deleting files
private:
    Database &db;
    Config &cfg;
    bool mkdir_recursive(std::string path);

    void rmdir_r(const std::string &path);//delete directory and all empty parent directories
    long get_target_free_bytes(void);//return the bytes that must be deleted
};

#endif
