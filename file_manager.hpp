#ifndef __FILE_MANAGER_HPP__
#define __FILE_MANAGER_HPP__

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
private:
    Database &db;
    Config &cfg;
    bool mkdir_recursive(std::string path);
};

#endif
