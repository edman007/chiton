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
#include <mutex>
#include <atomic>
#include <iostream>
#include <fstream>

class FileManager {
public:
    FileManager(Database &db, Config &cfg);

    //returns a valid path starting at start_time, writes out it's ID to int &file_id, if extend_file is true, the previous filename is used
    std::string get_next_path(long int &file_id, int camera, const struct timeval &start_time, bool extend_file = true);

    //returns a path to export a file to, updating the database to include the path
    std::string get_export_path(long int export_id, int camera, const struct timeval &start_time);

    //generates and writes path and name given start_time and extension, returns true on success
    //enters the image into the database if starttime is not NULL
    //if starttime is NULL, path is kept as is
    //name will be re-written to include the extension, if non-empty will become a prefix
    bool get_image_path(std::string &path, std::string &name, const std::string &extension, const struct timeval *start_time = NULL);

    //returns the real for the segments referenced as id and path name from the database
    std::string get_path(long int file_id, const std::string &db_path, const std::string &ext);

    //update metadata about the file
    bool update_file_metadata(long int file_id, struct timeval &end_time, long long end_byte, long long start_byte = 0, long long init_len = -1);

    void clean_disk(void);//clean up the disk by deleting files

    void delete_broken_segments(void);//looks for impossible segments and wipes them

    long rm_file(const std::string &path, const std::string &base = std::string("NULL"));//delete specific file (not a segment), returns number of bytes removed (-1 if nothing deleted)

    bool reserve_bytes(long bytes, int camera);//reserve bytes for camera

    long get_filesize(const std::string &path);//return the filesize of the file at path

    //open and return a fstream for name located in path, if there was a failure fstream.is_open() will fail, path must end in /
    std::ofstream get_fstream_write(const std::string &name, const std::string &path = "" , const std::string &base = "NULL");
    std::ifstream get_fstream_read(const std::string &name, const std::string &path = "" , const std::string &base = "NULL");
private:
    Database &db;
    Config &cfg;
    long bytes_per_segment;//estimate of segment size for our database to optimize our cleanup calls
    long min_free_bytes;//the config setting min-free-space as computed for the output-dir
    std::string last_filename;

    //global variables for cleanup and space reseverations
    static std::mutex cleanup_mtx;//lock when cleanup is in progress, locks just the clean_disk()
    static std::atomic<long> reserved_bytes;//total reserved bytes

    bool mkdir_recursive(std::string path);

    void rmdir_r(const std::string &path);//delete directory and all empty parent directories
    long get_target_free_bytes(void);//return the bytes that must be deleted
    long get_free_bytes(void);//return the free bytes on the disk
    long get_min_free_bytes(void);//return the miniumn free bytes on the disk
    long rm_segment(const std::string &base, const std::string &path, const std::string &id, const std::string &ext);//deletes a target segment, returns number of bytes removed
    long rm(const std::string &path);//delete a specific file and parent directories, returns negative if there was an error
    std::string get_date_path(int camera, const struct timeval &start_time);//returns a path in the form of <camera>/<YYYY>/<MM>/<DD>/<HH>
    std::string get_output_dir(void);//returns the output-dir cfg setting, with some fixups/sanity checks ensuring it always ends in a "/"
    std::string get_real_base(const std::string base);//given an input base, returns a real path that always ends in a /
    long clean_images(unsigned long start_time);//deletes images older than start_time, returns bytes deleted
};

#endif
