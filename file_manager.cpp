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
#include <sys/statvfs.h>
#include <dirent.h>
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

    //make sure the config value is correct
    std::string modified_base = std::string(base);
    if (modified_base == ""){
        modified_base = "./";
    } else if (modified_base.back() != '/'){
        modified_base += "/";
    }
    
    //make sure dir exists
    std::string path = modified_base + dir;
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

void FileManager::clean_disk(void){
    long target_clear = get_target_free_bytes();
    if (target_clear){
        LINFO("Cleaning the disk, removing " + std::to_string(target_clear));
        std::string sql = "SELECT v.id, v.path, c.value FROM videos AS v LEFT JOIN config AS c ON v.camera=c.camera AND c.name = 'output-dir' ORDER BY starttime ASC LIMIT 50";
        DatabaseResult* res = db.query(sql);
        while (target_clear > 0 && res && res->next_row()){
            struct stat statbuf;
            std::string target_file = res->get_field(2);
            if (target_file == "NULL"){
                target_file = cfg.get_value("output-dir");
            }
            if (target_file.back() != '/'){
                target_file += "/";
            } else {
                LINFO("Output dir ends in a /: " + target_file);
            }

            target_file += std::string(res->get_field(1));
            std::string target_dir = target_file;//copy the parent directory for after we delete all the content
            
            target_file += std::string(res->get_field(0)) + ".mp4";
            LINFO("Target: " + target_file);
            if (!stat(target_file.c_str(), &statbuf)){
                target_clear -= statbuf.st_size;
                //and delete it
                LINFO("Deleting " + target_file);
                if (unlink(target_file.c_str())){
                    LWARN("Cannot delete file " + target_file + " (" + std::to_string(errno) + ")");
                }
            } else {
                LWARN("Cannot stat " + target_file + " (" + std::to_string(errno) + ")");
            }

            //delete it from the database
            sql = "DELETE FROM videos WHERE id = " + res->get_field(0);
            DatabaseResult* del_res = db.query(sql);
            delete del_res;

            rmdir_r(target_dir);//and delete any empty parents

        }
        delete res;
    }

}

long FileManager::get_target_free_bytes(void){
    std::string check_path = cfg.get_value("output-dir");
    struct statvfs info;
    if (!statvfs(check_path.c_str(), &info)){
        long free_bytes = info.f_bsize * info.f_bavail;
        long min_free = cfg.get_value_long("min-free-space");
        if (cfg.get_value("min-free-space").find('%') != std::string::npos){
            double min_freed = cfg.get_value_double("min-free-space");//absolute bytes are set
            min_freed /= 100;
            long total_bytes = info.f_bsize * (info.f_blocks - info.f_bfree + info.f_bavail);//ignore root's space we can't use when calculating filesystem space
            min_free = total_bytes * min_freed;
            LWARN("Percent mode, target is " + std::to_string(min_free));
            if (min_free < 0){
                min_free = 0;//they set something wrong...I don't know, this will result in the defualt
            }
            
        }
        
        //absolute byte mode
        if (!min_free){
            min_free = DEFAULT_MIN_FREE_SPACE;
        }
        if (free_bytes < min_free){
            return min_free - free_bytes;
        }
    } else {
        LWARN("Failed to get filesystem info for " + check_path + " ( " +std::to_string(errno)+" ) will not clean from this directory");
    }
    return 0;
}

void FileManager::rmdir_r(const std::string &path){
    LINFO("Deleting " + path);
    DIR *dir = opendir(path.c_str());
    if (dir){
        struct dirent* dir_info;
        bool dir_is_empty = true;
        while ((dir_info = readdir(dir))){
            if (std::string(dir_info->d_name) == "." || std::string(dir_info->d_name) == ".."){
                continue;
            }
            dir_is_empty = false;
            break;
        }
        closedir(dir);
        if (dir_is_empty){
            //dir is empty, delete it
            if (rmdir(path.c_str())){
                LWARN("Failed to delete empty directory " + path + " (" + std::to_string(errno) + ")");
            } else {
                if (path.length() > 2){
                    std::string parent_dir = path.substr(0, path.find_last_of('/', path.length() - 2));
                    rmdir_r(parent_dir);
                }
            }
        }
    } else {
        LWARN("Could not open directory " + path + " to delete it (" +std::to_string(errno) + ")");
    }
}
