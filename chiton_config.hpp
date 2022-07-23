#ifndef __CHITON_CONFIG_HPP__
#define __CHITON_CONFIG_HPP__
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

//we like new stuff
#define _XOPEN_SOURCE 700
#define CL_TARGET_OPENCL_VERSION 300

#include <string>
#include <map>

/* Autoconf parameters */
#include "config_build.hpp"
#include "database.hpp"

//we have to have valid values for these, these are the defaults when the user sets a bad value
const long DEFAULT_SECONDS_PER_SEGMENT = 6;//Apple recommends 6 seconds per file to make live streaming reasonable
const long DEFAULT_MIN_FREE_SPACE = 1073741824;//1G in bytes
const std::string EXPORT_EXT = ".mp4";

class Config {
public:
    Config();//init an empty Config database
    Config(const Config &src);//init with the same db as some other Config
    bool load_config(const std::string& path);//load the config from the path
    bool load_camera_config(int camera, Database &db);//load the config from the database for a specific camera
    
    const std::string& get_value(const std::string& key);
    int get_value_int(const std::string& key);//returns the value as an int
    long get_value_long(const std::string& key);//returns the value as a long
    double get_value_double(const std::string& key);//returns the value as an double
    
    void set_value(const std::string& key, const std::string& value);

private:
    std::map<std::string,std::string> cfg_db;
    const std::string EMPTY_STR = "";

    const std::string & get_default_value(const std::string& key);
};
#endif
