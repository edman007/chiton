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
 *   Copyright 2020-2022 Ed Martin <edman007@edman007.com>
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
#include "stream_writer.hpp"
#include "system_controller.fwd.hpp"

class FileManager {
public:
    FileManager(SystemController &sys);
    FileManager(SystemController &sys, Config &cfg);//with alternate config

    void init(void);//init's the FileManager

    //returns a valid path starting at start_time, writes out it's ID to int &file_id, if extend_file is true, the previous filename is used
    std::string get_next_path(long int &file_id, int camera, const struct timeval &start_time, bool extend_file = false);

    //returns a path to export a file to, updating the database to include the path
    std::string get_export_path(long int export_id, int camera, const struct timeval &start_time);

    //generates and writes path and name given start_time and extension, returns true on success
    //enters the image into the database if starttime is not NULL
    //if starttime is NULL, path is kept as is
    //name will be re-written to include the extension, if non-empty will become a prefix
    //if file_id is non-null, then the ID is written to it if it was written to the db
    bool get_image_path(std::string &path, std::string &name, const std::string &extension, const struct timeval *start_time = NULL, long *file_id = NULL);

    //returns the real for the segments referenced as id and path name from the database
    std::string get_path(long long name, const std::string &db_path, const std::string &ext);

    //update metadata about the file
    bool update_file_metadata(long int file_id, struct timeval &end_time, long long end_byte, const StreamWriter &out_file, long long start_byte = 0, long long init_len = -1);

    void clean_disk(void);//clean up the disk by deleting files

    void delete_broken_segments(void);//looks for impossible segments and wipes them

    //delete specific file (not a segment), returns number of bytes removed (-1 if nothing deleted)
    long long rm_file(const std::string &path, const std::string &base = std::string("NULL"));

    bool reserve_bytes(long long bytes, int camera);//reserve bytes for camera

    long long get_filesize(const std::string &path);//return the filesize of the file at path

    //open and return a fstream for name located in path, if there was a failure fstream.is_open() will fail, path must end in /
    std::ofstream get_fstream_write(const std::string &name, const std::string &path = "" , const std::string &base = "NULL");
    std::ifstream get_fstream_read(const std::string &name, const std::string &path = "" , const std::string &base = "NULL");
private:
    Database &db;
    Config &cfg;
    SystemController &sys;
    long long bytes_per_segment;//estimate of segment size for our database to optimize our cleanup calls
    long long min_free_bytes;//the config setting min-free-space as computed for the output-dir
    std::string last_filename;//when extending files, holds the current filename
    std::string last_dir;//when extending files, holds the current directory

    //global variables for cleanup and space reseverations
    static std::mutex cleanup_mtx;//lock when cleanup is in progress, locks just the clean_disk()
    static std::atomic<long long> reserved_bytes;//total reserved bytes

    bool mkdir_recursive(std::string path);

    void rmdir_r(const std::string &path);//delete directory and all empty parent directories
    long long get_target_free_bytes(void);//return the bytes that must be deleted
    long long get_free_bytes(void);//return the free bytes on the disk
    long long get_min_free_bytes(void);//return the miniumn free bytes on the disk
    long long rm_segment(const std::string &base, const std::string &path, const std::string &id, const std::string &ext);//deletes a target segment, returns number of bytes removed
    long long rm(const std::string &path);//delete a specific file and parent directories, returns negative if there was an error
    std::string get_date_path(int camera, const struct timeval &start_time);//returns a path in the form of <camera>/<YYYY>/<MM>/<DD>/<HH>
    std::string get_output_dir(void);//returns the output-dir cfg setting, with some fixups/sanity checks ensuring it always ends in a "/"
    std::string get_real_base(const std::string base);//given an input base, returns a real path that always ends in a /
    long long clean_images(unsigned long long start_time);//deletes images older than start_time, returns bytes deleted
    long clean_events(unsigned long long start_time);//deletes events older than start_time, returns number deleted
};

#endif
