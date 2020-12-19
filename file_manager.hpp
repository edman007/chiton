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
    std::string get_next_path(long int &file_id, int camera, const struct timeval &start_time);

    //returns a path to export a file to, updating the database to include the path
    std::string get_export_path(long int export_id, int camera, const struct timeval &start_time);

    //returns the real for the segments referenced as id and path name from the database
    std::string get_path(long int file_id, const std::string &db_path);

    //update metadata about the file
    bool update_file_metadata(long int file_id, struct timeval &end_time);

    void clean_disk(void);//clean up the disk by deleting files

    void delete_broken_segments(void);//looks for impossible segments and wipes them

    long rm_file(const std::string &path);//delete specific file (not a segment), returns number of bytes removed (-1 if nothing deleted)
private:
    Database &db;
    Config &cfg;
    bool mkdir_recursive(std::string path);

    void rmdir_r(const std::string &path);//delete directory and all empty parent directories
    long get_target_free_bytes(void);//return the bytes that must be deleted
    long rm_segment(const std::string &base, const std::string &path, const std::string &id);//deletes a target segment, returns number of bytes removed
    long rm(const std::string &path);//delete a specific file and parent directories, returns negative if there was an error
    std::string get_date_path(int camera, const struct timeval &start_time);//returns a path in the form of <camera>/<YYYY>/<MM>/<DD>/<HH>
    std::string get_output_dir(void);//returns the output-dir cfg setting, with some fixups/sanity checks ensuring it always ends in a "/"
};

#endif
