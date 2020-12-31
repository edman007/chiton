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

std::string FileManager::get_next_path(long int &file_id, int camera, const struct timeval &start_time){
    const std::string base = get_output_dir();
    const std::string dir = get_date_path(camera, start_time);
    std::string ptime = std::to_string(Util::pack_time(start_time));
    //make sure dir exists
    std::string path = base + dir;
    mkdir_recursive(path);
    
    std::string sql = "INSERT INTO videos (path, starttime, camera) VALUES ('" + dir + "/'," + ptime + ", " + std::to_string(camera) + " )";    
    DatabaseResult* res = db.query(sql, NULL, &file_id);
    delete res;
    
    return path + "/" + std::to_string(file_id) + FILE_EXT;
}

std::string FileManager::get_export_path(long int export_id, int camera, const struct timeval &start_time){
    const std::string base = get_output_dir();
    const std::string dir = get_date_path(camera, start_time);

    //make sure dir exists
    std::string path = base + dir;
    mkdir_recursive(path);

    std::string sql = "UPDATE exports SET path = '" + dir + "/' WHERE id = " + std::to_string(export_id);
    DatabaseResult* res = db.query(sql);
    delete res;

    return path + "/" + std::to_string(export_id) + EXPORT_EXT;
}
//returns the path for the file referenced as id
std::string FileManager::get_path(long int id, const std::string &db_path){
    std::string path;
    path = db_path + std::to_string(id) + FILE_EXT;
    return get_output_dir() + path;
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
        std::string sql = "SELECT v.id, v.path, c.value FROM videos AS v LEFT JOIN config AS c ON v.camera=c.camera AND c.name = 'output-dir' AND locked = 0 "
            " ORDER BY starttime ASC LIMIT 50";

        DatabaseResult* res = db.query(sql);
        while (target_clear > 0 && res && res->next_row()){
            long rm_size = 0;//size of file deleted
            const std::string &id = res->get_field(0);
            rm_size = rm_segment(res->get_field(2), res->get_field(1), id);
            if (rm_size > 0){
                target_clear -= rm_size;
            }
            //delete it from the database
            sql = "DELETE FROM videos WHERE id = " + id;
            DatabaseResult* del_res = db.query(sql);
            delete del_res;
        }
        delete res;
    }

}

long FileManager::rm_segment(const std::string &base, const std::string &path, const std::string &id){
    long filesize = 0;
    std::string target_file = path;

    target_file += id + FILE_EXT;
    filesize = rm_file(target_file, base);
    return filesize;
}

void FileManager::delete_broken_segments(void){
    LINFO("Removing Broken Segments");

    //get the current time, plus some time
    struct timeval curtime;
    Util::get_videotime(curtime);
    long broken_offset = cfg.get_value_long("broken-time-offset");
    if (broken_offset > 0){
        curtime.tv_sec += broken_offset;
    }
    std::string future_time = std::to_string(Util::pack_time(curtime));

    std::string sql = "SELECT v.id, v.path, c.value FROM videos AS v LEFT JOIN config AS c ON v.camera=c.camera AND c.name = 'output-dir' WHERE "
        "v.endtime < v.starttime OR v.starttime > " + future_time;
    DatabaseResult* res = db.query(sql);
    if (res && res->num_rows() > 0){
        LWARN("Found " + std::to_string(res->num_rows()) + " broken  segments, deleting them");
    }
    while (res && res->next_row()){
        const std::string &id = res->get_field(0);
        rm_segment(res->get_field(2), res->get_field(1), id);
        //delete it from the database
        sql = "DELETE FROM videos WHERE id = " + id;
        DatabaseResult* del_res = db.query(sql);
        delete del_res;
    }
    delete res;
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
            if (min_free < 0){
                min_free = 0;//they set something wrong...I don't know, this will result in the defualt
            }
            
        }
        
        //absolute byte mode
        if (min_free <= 1024){
            LWARN("min-free-space was set too small");
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

bool FileManager::update_file_metadata(long int file_id, struct timeval &end_time){
    long affected;
    std::string ptime = std::to_string(Util::pack_time(end_time));
    std::string sql = "UPDATE videos SET endtime = " + ptime + " WHERE id = " + std::to_string(file_id) + " ";    
    DatabaseResult* res = db.query(sql, &affected, NULL);
    delete res;

    return affected == 1;
    
}

long FileManager::rm(const std::string &path){
    struct stat statbuf;
    if (!stat(path.c_str(), &statbuf)){
        //and delete it
        LINFO("Deleting " + path);
        if (unlink(path.c_str())){
            LWARN("Cannot delete file " + path + " (" + std::to_string(errno) + ")");
            return -2;
        }
        return statbuf.st_size;
    } else {
        LWARN("Cannot stat " + path + " (" + std::to_string(errno) + ")");
        return -1;
    }
}

long FileManager::rm_file(const std::string &path, const std::string &base/* = std::string("NULL")*/){
    std::string real_base = base;
    if (base == "NULL"){
        real_base = get_output_dir();
    } else if (real_base.back() != '/'){
        real_base += "/";
    }

    long filesize = rm(real_base + path);
    std::string dir = real_base;
    dir += path.substr(0, path.find_last_of('/', path.length()));
    rmdir_r(dir);
    return filesize;
}


std::string FileManager::get_date_path(int camera, const struct timeval &start_time){
    VideoDate date;
    Util::get_time_parts(start_time, date);
    std::string dir = std::to_string(camera) + "/"
        + std::to_string(date.year) + "/"
        + std::to_string(date.month) + "/"
        + std::to_string(date.day) + "/"
        + std::to_string(date.hour);
    return dir;
}

std::string FileManager::get_output_dir(void){
    const std::string base = cfg.get_value("output-dir");

    //make sure the config value is correct
    std::string modified_base = std::string(base);
    if (modified_base == ""){
        modified_base = "./";
    } else if (modified_base.back() != '/'){
        modified_base += "/";
    }
    return modified_base;
}
