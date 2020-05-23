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
#include "file_manager.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#include "util.hpp"

std::string FileManager::get_next_path(long int &file_id, int camera, struct timeval &start_time){
    const std::string &base = cfg.get_value("output-dir");
    VideoDate date;
    Util::get_time_parts(start_time, date);
    std::string ptime = std::to_string(Util::pack_time(start_time));
    const std::string dir = std::to_string(camera) + "/"
        + std::to_string(date.year) + "/"
        + std::to_string(date.month) + "/"
        + std::to_string(date.day) + "/"
        + std::to_string(date.hour);


    //make sure dir exists
    std::string path = base + dir;
    mkdir_recursive(path);
    
    std::string sql = "INSERT INTO videos (path, starttime, camera) VALUES ('" + dir + "/'," + ptime + ", " + std::to_string(camera) + " )";    
    DatabaseResult* res = db.query(sql, NULL, &file_id);
    delete res;
    
    return path + "/" + std::to_string(file_id) + ".mp4";
}

//returns the path for the file referenced as id
std::string FileManager::get_path(long int id){
    std::string path;
    std::string sql = "SELECT path from videos WHERE id = " + std::to_string(id);
    DatabaseResult *res = db.query(sql);
    if (res->next_row()){
        path = std::string(res->get_field(0)) + std::to_string(id) + ".mp4";
    }
    delete res;
    return path;
}

bool FileManager::mkdir_recursive(std::string path){
    struct stat buf;
    if (stat(path.c_str(), &buf)){
        //does not exist, make it if the parent exists:
        auto new_len = path.find_last_of("/", path.length() - 1);
        auto parent_dir = path.substr(0, new_len);
        if (new_len != std::string::npos || !parent_dir.compare("")){
            if (!mkdir_recursive(parent_dir)){
                return false;
            }
        }
        if (mkdir(path.c_str(), 0755)){
            LERROR("Cannot make directory " + path + " (errno: " + std::to_string(errno) + ")");
            return false;
        }
        return true;
    } else if (buf.st_mode | S_IFDIR) {
        return true;
    }
    return false;
    
}
