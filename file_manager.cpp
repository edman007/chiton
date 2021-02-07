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

//static globals
std::mutex FileManager::cleanup_mtx;//lock when cleanup is in progress, locks just the clean_disk() to prevent iterating over the same database results twice
std::atomic<long> FileManager::reserved_bytes(0);

FileManager::FileManager(Database &db, Config &cfg) : db(db), cfg(cfg) {
    bytes_per_segment = 1024*1024;//1M is our default guess
    min_free_bytes = -1;

    //we correct this if it's wrong now, so we can assume it's always right
    const std::string &ext = cfg.get_value("output-extension");
    if (ext != ".ts" && ext != ".mp4"){
        LWARN("output-extension MUST be .ts or .mp4, using .mp4");
        cfg.set_value("output-extension", ".mp4");
    }
}

std::string FileManager::get_next_path(long int &file_id, int camera, const struct timeval &start_time, bool extend_file /* = false */){
    const std::string base = get_output_dir();
    const std::string dir = get_date_path(camera, start_time);
    std::string ptime = std::to_string(Util::pack_time(start_time));
    std::string name;
    if (extend_file && !last_filename.empty()){
        name = last_filename;
    } else {
        name = ptime;
        last_filename = name;
    }
    //make sure dir exists
    std::string path = base + dir;
    mkdir_recursive(path);
    const std::string &ext = cfg.get_value("output-extension");

    std::string sql = "INSERT INTO videos (path, starttime, camera, extension, name) VALUES ('" + db.escape(dir + "/") + "'," + ptime + ", " + std::to_string(camera) + ",'"
        + db.escape(ext) + "'," + name + " )";
    DatabaseResult* res = db.query(sql, NULL, &file_id);
    delete res;
    
    return path + "/" + name + ext;
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
std::string FileManager::get_path(long int id, const std::string &db_path, const std::string &ext){
    std::string path;
    path = db_path + std::to_string(id) + ext;
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
    cleanup_mtx.lock();
    long target_clear = get_target_free_bytes();
    if (target_clear){
        LINFO("Cleaning the disk, removing " + std::to_string(target_clear));

        //estimate the number of segments, add 10
        long segment_count = target_clear/bytes_per_segment + 10;
        std::string sql = "SELECT v.camera, v.path, c.value, MIN(v.starttime), v.extension, v.name FROM videos AS v LEFT JOIN config AS c ON v.camera=c.camera AND c.name = 'output-dir' "
            " GROUP BY v.name, v.camera HAVING MAX(v.locked) = 0  ORDER BY starttime ASC LIMIT " + std::to_string(segment_count);

        segment_count = 0;
        long actual_segment_bytes = 0;
        unsigned long oldest_record = 0;
        DatabaseResult* res = db.query(sql);
        while (target_clear > 0 && res && res->next_row()){
            segment_count++;
            long rm_size = 0;//size of file deleted
            const std::string &name = res->get_field(5);
            rm_size = rm_segment(res->get_field(2), res->get_field(1), name, res->get_field(4));
            if (rm_size > 0){
                target_clear -= rm_size;
                actual_segment_bytes += rm_size;
            }
            //delete it from the database
            sql = "DELETE FROM videos WHERE name = " + res->get_field(5) + " AND camera = " + res->get_field(0);
            DatabaseResult* del_res = db.query(sql);
            delete del_res;

            //record the starttime of this
            oldest_record = res->get_field_long(3);
        }
        delete res;

        if (segment_count > 0){
            bytes_per_segment = actual_segment_bytes/segment_count;
            if (bytes_per_segment < 100){
                //probably some issue, set to 1M
                LINFO("Deleting Segments, average segment size appears to be below 100b");
                bytes_per_segment = 1024*1024;
            }
            LDEBUG("Average segment was " + std::to_string(bytes_per_segment) + " bytes");

            //clean the images
            target_clear -= clean_images(oldest_record);
        }

        if (target_clear > 0){
            //this may mean that we are not going to clear out the reserved space
            //but we don't add it to the reserved count because it could be more than the reserved space
            //we don't want it to snowball so we just print a warning
            LWARN(std::to_string(target_clear) + " bytes were not removed from the disk as requested");
        }
    }
    cleanup_mtx.unlock();
}

long FileManager::rm_segment(const std::string &base, const std::string &path, const std::string &id, const std::string &ext){
    long filesize = 0;
    std::string target_file = path;

    target_file += id + ext;
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

    std::string sql = "SELECT v.id, v.path, c.value, v.extension FROM videos AS v LEFT JOIN config AS c ON v.camera=c.camera AND c.name = 'output-dir' WHERE "
        "v.endtime < v.starttime OR v.starttime > " + future_time;
    DatabaseResult* res = db.query(sql);
    if (res && res->num_rows() > 0){
        LWARN("Found " + std::to_string(res->num_rows()) + " broken  segments, deleting them");
    }
    while (res && res->next_row()){
        const std::string &id = res->get_field(0);
        rm_segment(res->get_field(2), res->get_field(1), id, res->get_field(3));
        //delete it from the database
        sql = "DELETE FROM videos WHERE id = " + id;
        DatabaseResult* del_res = db.query(sql);
        delete del_res;
    }
    delete res;
}

long FileManager::get_target_free_bytes(void){
    long free_bytes = get_free_bytes();
    long min_free = get_min_free_bytes();
    min_free += reserved_bytes.exchange(0);
    if (free_bytes < min_free){
        return min_free - free_bytes;
    }
    return 0;
}

long FileManager::get_free_bytes(void){
    std::string check_path = cfg.get_value("output-dir");
    struct statvfs info;
    if (!statvfs(check_path.c_str(), &info)){
        long free_bytes = info.f_bsize * info.f_bavail;
        return free_bytes;
    } else {
        LWARN("Failed to get filesystem info for " + check_path + " ( " +std::to_string(errno)+" ) will assume it is full");
    }
    return 0;
}

long FileManager::get_min_free_bytes(void){
    if (min_free_bytes > 0){
        return min_free_bytes;
    }
    //we only perform this syscall on the first call
    long min_free = cfg.get_value_long("min-free-space");
    if (cfg.get_value("min-free-space").find('%') != std::string::npos){
        std::string check_path = cfg.get_value("output-dir");
        struct statvfs info;
        if (!statvfs(check_path.c_str(), &info)){

            double min_freed = cfg.get_value_double("min-free-space");//absolute bytes are set
            min_freed /= 100;
            long total_bytes = info.f_bsize * (info.f_blocks - info.f_bfree + info.f_bavail);//ignore root's space we can't use when calculating filesystem space
            min_free = total_bytes * min_freed;
            if (min_free < 0){
                min_free = 0;//they set something wrong...I don't know, this will result in the defualt
            }
        } else {
            min_free = 0;//use the default
        }
        
    }

    //absolute byte mode
    if (min_free <= 1024){
        LWARN("min-free-space was set too small");
        min_free = DEFAULT_MIN_FREE_SPACE;
    }
    min_free_bytes = min_free;
    return min_free_bytes;
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

bool FileManager::update_file_metadata(long int file_id, struct timeval &end_time, long long end_byte, long long start_byte /*= 0 */, long long init_len /*= -1*/){
    long affected;
    std::string ptime = std::to_string(Util::pack_time(end_time));
    std::string sql = "UPDATE videos SET endtime = " + ptime;
    if (init_len >= 0){
        sql += ", init_byte = " + std::to_string(init_len);
    }
    if (start_byte >= 0){
        sql += ", start_byte = " + std::to_string(start_byte);
    }
    if (end_byte > 0){
        sql += ", end_byte = " + std::to_string(end_byte);
    }

    sql += " WHERE id = " + std::to_string(file_id);
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

long FileManager::get_filesize(const std::string &path){
    struct stat statbuf;
    if (!stat(path.c_str(), &statbuf)){
        return statbuf.st_size;
    }
    return -1;
}

long FileManager::rm_file(const std::string &path, const std::string &base/* = std::string("NULL")*/){
    std::string real_base = get_real_base(base);

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

bool FileManager::reserve_bytes(long bytes, int camera){
    long free_space = get_free_bytes();
    long min_space = get_min_free_bytes();
    reserved_bytes += bytes;
    free_space -= min_space/2;
    free_space -= reserved_bytes;
    //if this reservation would eat up more than half our free space we clean the disk
    if (free_space < 0){
        clean_disk();
        return true;
    }
    //otherwise we leave it as is
    return true;
}

std::ofstream FileManager::get_fstream_write(const std::string &name, const std::string &path /* = "" */, const std::string &base/* = "NULL" */){
    std::ofstream s;
    std::string real_base = get_real_base(base);
    mkdir_recursive(real_base + path);
    s.open(real_base + path + name, std::ios::binary | std::fstream::out | std::fstream::trunc);
    if (!s.is_open()){
        LWARN("Failed to open '" + real_base + path + name + "', " + std::to_string(errno));
    }
    return s;
}

std::ifstream FileManager::get_fstream_read(const std::string &name, const std::string &path /* = "" */, const std::string &base/* = "NULL"*/){
    std::ifstream s;
    std::string real_base = get_real_base(base);
    s.open(real_base + path + name, std::ios::binary | std::fstream::in);
    if (!s.is_open()){
        LWARN("Failed to open '" + real_base + path + name + "', " + std::to_string(errno));
    }
    return s;
}

std::string FileManager::get_real_base(const std::string base){
    std::string real_base;
    if (base == "NULL"){
        real_base = get_output_dir();
    } else if (real_base.back() != '/'){
        real_base += "/";
    }
    return real_base;
}

bool FileManager::get_image_path(std::string &path, std::string &name, const std::string &extension, const struct timeval *start_time /* = NULL */){
    if (start_time != NULL){
        path = get_date_path(cfg.get_value_int("camera-id"), *start_time) + "/";
        if (name != ""){
            name += "-";
        }
        std::string ptime = std::to_string(Util::pack_time(*start_time));
        std::string sql = "INSERT INTO images (camera, path, prefix, extension, starttime) VALUES (" +
            cfg.get_value("camera-id") + ",'" + db.escape(path) + "','" + db.escape(name) + "','" + db.escape(extension) + "'," + ptime + ")";
        long file_id = 0;
        DatabaseResult* res = db.query(sql, NULL, &file_id);
        if (res){
            name += std::to_string(file_id) + extension;
            delete res;
        } else {
            return false;
        }
    } else {
        name += extension;
    }

    return true;
}

long FileManager::clean_images(unsigned long start_time){
    if (start_time == 0){
        return 0;//bail if it's invalid
    }

    long bytes_deleted = 0;

    std::string sql = "SELECT id, path, prefix, extension, c.value FROM images as i LEFT JOIN config AS c ON i.camera=c.camera AND c.name = 'output-dir' "
        " WHERE starttime < " + std::to_string(start_time);
    DatabaseResult *res = db.query(sql);
    while (res && res->next_row()){
        std::string img_id = res->get_field(0);
        std::string path = res->get_field(1);
        std::string name = res->get_field(2) + img_id + res->get_field(3);
        std::string base = res->get_field(4);
        long del_bytes = rm_file(path + name, base);
        if (del_bytes > 0){
            bytes_deleted++;
        }
        LWARN("Deleting image " + name);
        //delete it from the database
        sql = "DELETE FROM images WHERE id = " + img_id;
        DatabaseResult* del_res = db.query(sql);
        delete del_res;
    }
    return bytes_deleted;
}
