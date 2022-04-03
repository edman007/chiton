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
 *   Copyright 2020-2021 Ed Martin <edman007@edman007.com>
 *
 **************************************************************************
 */

#include "database_manager.hpp"
#include "util.hpp"

//we increment Major when we break backwards compatibility
static const int CURRENT_DB_VERSION_MAJOR = 1;
//we increment minor when we add a backwards compatible version
static const int CURRENT_DB_VERSION_MINOR = 4;
static const std::string CURRENT_DB_VERSION = std::to_string(CURRENT_DB_VERSION_MAJOR) + "." + std::to_string(CURRENT_DB_VERSION_MINOR);

DatabaseManager::DatabaseManager(Database &db) : db(db) {

}

bool DatabaseManager::initilize_db(){
    const std::string config_tbl = "CREATE TABLE config ( "
        "name varchar(40) COLLATE utf8mb4_unicode_ci NOT NULL, "
        "value varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL, "
        "camera int(11) NOT NULL, "
        "UNIQUE KEY camera (camera,name) "
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";
    const std::string videos_tbl = "CREATE TABLE videos ( "
        "id int(11) unsigned NOT NULL AUTO_INCREMENT, "
        "path varchar(255) COLLATE utf8mb4_unicode_ci DEFAULT NULL, "
        "starttime bigint(20) NOT NULL, "
        "endtime bigint(20) DEFAULT NULL, "
        "camera int(11) NOT NULL, "
        "`locked` tinyint(1) NOT NULL DEFAULT 0, "
        "`extension` ENUM('.ts','.mp4') CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '.ts', "
        "`name` BIGINT NOT NULL, "
        "`init_byte` INT NULL DEFAULT NULL, "
        "`start_byte` INT NULL DEFAULT NULL, "
        "`end_byte` INT NULL DEFAULT NULL, "
        "`codec` varchar(255) COLLATE utf8mb4_unicode_ci DEFAULT NULL, "
        "`av_type` ENUM('audio', 'video', 'audiovideo') CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT 'audiovideo', "
        "`width` INT NULL DEFAULT NULL, "
        "`height` INT NULL DEFAULT NULL, "
        "`framerate` FLOAT NULL DEFAULT NULL, "
        "PRIMARY KEY (id,camera,starttime), "
        "KEY endtime (endtime), "
        "KEY starttime (starttime), "
        "KEY camera (camera) "
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";
    const std::string exports_tbl = "CREATE TABLE `exports` ( "
        "`id` int(10) unsigned NOT NULL AUTO_INCREMENT, "
        "`camera` int(11) NOT NULL, "
        "`path` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '', "
        "`starttime` bigint(20) NOT NULL, "
        "`endtime` bigint(20) NOT NULL, "
        "`progress` int(11) NOT NULL DEFAULT 0, "
        "PRIMARY KEY (`id`), "
        "KEY `camera` (`camera`,`starttime`,`progress`) "
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";
    const std::string images_tbl = "CREATE TABLE `images` ( "
        "`id` int(11) NOT NULL AUTO_INCREMENT, "
        " camera int(11) NOT NULL, "
        "`path` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL, "
        "`prefix` varchar(60) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL, "
        "`extension` varchar(60) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL, "
        "`starttime` bigint(20) NOT NULL, "
        "PRIMARY KEY (`id`), "
        "KEY starttime (starttime) "
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";
    const std::string events_tbl ="CREATE TABLE `events` ("
        "`id` int(11) NOT NULL AUTO_INCREMENT, "
        "`camera` int(11) NOT NULL, "
        "`img` int(11) DEFAULT NULL, "
        "`source` varchar(64) NOT NULL, "
        "`starttime` bigint(20) NOT NULL, "
        "`x0` int(11) NOT NULL, "
        "`y0` int(11) NOT NULL, "
        "`x1` int(11) NOT NULL, "
        "`y1` int(11) NOT NULL, "
        "`score` float NOT NULL, "
        "PRIMARY KEY (`id`), "
        "KEY camera (`camera`, `source`, `starttime`) "
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci ";

    //create the config table
    DatabaseResult *res = db.query(config_tbl);
    bool ret = true;
    if (res){
        delete res;
    } else {
        LFATAL("Could not create config table");
        ret = false;
    }

    //create the videos table
    if (ret){
        res = db.query(videos_tbl);
        if (res){
            delete res;
        } else {
            LFATAL("Could not create videos table");
            ret = false;
        }
    }

    //create the exports table
    if (ret){
        res = db.query(exports_tbl);
        if (res){
            delete res;
        } else {
            LFATAL("Could not create exports table");
            ret = false;
        }
    }

    //create the images table
    if (ret){
        res = db.query(images_tbl);
        if (res){
            delete res;
        } else {
            LFATAL("Could not create images table");
            ret = false;
        }
    }

    //create the events table
    if (ret){
        res = db.query(events_tbl);
        if (res){
            delete res;
        } else {
            LFATAL("Could not create events table");
            ret = false;
        }
    }

    if (ret){
        return set_latest_version();
    }
    return ret;

}

bool DatabaseManager::check_database(void){
    std::string sql = "SELECT value FROM config WHERE name = 'database-version' AND camera = -1";
    DatabaseResult *res = db.query(sql);
    bool ret = false;
    if (res && res->next_row()){
        std::string cur_version = res->get_field(0);
        int cur_major;

        int cur_minor;
        try {
            cur_major = std::stoi(cur_version.substr(0, cur_version.find('.')));
            cur_minor = std::stoi(cur_version.substr(cur_version.find('.') + 1));
        } catch (std::invalid_argument &e){
            LDEBUG("Database Version is invalid");
            cur_major = INT_MAX;
            cur_minor = INT_MAX;
        } catch (std::out_of_range &e) {
            LDEBUG("Database Version is out of range");
            cur_major = INT_MAX;
            cur_minor = INT_MAX;
        }
        if (cur_major > CURRENT_DB_VERSION_MAJOR){
            LFATAL("Error, current database version " + cur_version + " is incompible with this version of chiton");
        } else if (cur_major < CURRENT_DB_VERSION_MAJOR){
            if (!upgrade_database(cur_major, cur_minor)){
                LFATAL("The database needs an upgrade, but the upgrade from " + cur_version + " to " + CURRENT_DB_VERSION);//shouldn't happen
            } else {
                ret = true;
            }
            //normally we would just upgrade...but no such upgrade is possible
        } else {//equal
            if (cur_minor > CURRENT_DB_VERSION_MINOR){
                LWARN("Current database version is newer than expected (" + cur_version + " -> " + CURRENT_DB_VERSION
                      + "), continuing without changing the database version");
                ret = true;//this is not an issue
            } else if (cur_minor < CURRENT_DB_VERSION_MINOR){
                if (!upgrade_database(cur_major, cur_minor)){
                    LFATAL("The database needs an upgrade, but no other version is known, is " + cur_version + " but expected " + CURRENT_DB_VERSION);//shouldn't happen
                } else {
                    ret = true;
                }
            } else {//equal
                LINFO("Detected latest database version " + CURRENT_DB_VERSION);
                ret = true;
            }
        }
    } else {
        //no database version? then we try initilizing it
        ret = initilize_db();
    }
    delete res;
    return ret;
}

bool DatabaseManager::set_latest_version(void){
    std::string sql = "INSERT INTO config (camera, name, value) VALUES (-1, 'database-version', '"
        + CURRENT_DB_VERSION + "') ON DUPLICATE KEY UPDATE "
        + " value = '" + CURRENT_DB_VERSION + "'";
    long affected_rows = 0;
    DatabaseResult *res = db.query(sql, &affected_rows, NULL);
    bool ret = false;
    if (res && affected_rows){
        LINFO("Database Upgraded to " + CURRENT_DB_VERSION);
        ret = true;
    } else {
        LFATAL("Failed to update database version");
    }
    delete res;
    return ret;
}

bool DatabaseManager::upgrade_database(int major, int minor){
    if (1 == major){
        switch (minor){
        case 0:
            return upgrade_from_1_0();
        case 1:
            return upgrade_from_1_1();
        case 2:
            return upgrade_from_1_2();
        case 3:
            return upgrade_from_1_3();
        default:
            return false;
        }
    }
    return false;
}

bool DatabaseManager::upgrade_from_1_0(void){
    const std::string alter_query = "ALTER TABLE `videos` ADD `locked` BOOLEAN NOT NULL DEFAULT FALSE AFTER `camera`";
    const std::string exports_tbl = "CREATE TABLE `exports` ( "
        "`id` int(10) unsigned NOT NULL AUTO_INCREMENT, "
        "`camera` int(11) NOT NULL, "
        "`path` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '', "
        "`starttime` bigint(20) NOT NULL, "
        "`endtime` bigint(20) NOT NULL, "
        "`progress` int(11) NOT NULL DEFAULT 0, "
        "PRIMARY KEY (`id`), "
        "KEY `camera` (`camera`,`starttime`,`progress`) "
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";

    DatabaseResult *res = db.query(alter_query);
    bool ret = true;
    if (res){
        delete res;
    } else {
        LWARN("Database Upgrade Failed - " + alter_query);
        ret = false;
    }

    //create the exports table
    if (ret){
        res = db.query(exports_tbl);
        if (res){
            delete res;
        } else {
            LFATAL("Could not create exports table");
            ret = false;
        }
    }

    if (ret){
        return upgrade_from_1_1();
    } else {
        return ret;
    }
}

bool DatabaseManager::upgrade_from_1_1(void){
    const std::string images_tbl = "CREATE TABLE `images` ( "
        "`id` int(11) NOT NULL AUTO_INCREMENT, "
        "camera int(11) NOT NULL, "
        "`path` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL, "
        "`prefix` varchar(60) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL, "
        "`extension` varchar(60) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL, "
        "`starttime` bigint(20) NOT NULL, "
        "PRIMARY KEY (`id`), "
        "KEY starttime (starttime) "
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";
    const std::string video_alter = "ALTER TABLE `videos` ADD `extension` ENUM('.ts','.mp4') CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '.ts' AFTER `locked`, "
        " ADD `name` BIGINT NOT NULL AFTER `extension`, "
        " ADD `init_byte` INT NULL DEFAULT NULL AFTER `name`, "
        " ADD `start_byte` INT NULL DEFAULT NULL AFTER `init_byte`, "
        " ADD `end_byte` INT NULL DEFAULT NULL AFTER `start_byte`";
    const std::string video_update = "UPDATE videos SET name = id";

    bool ret = true;
    DatabaseResult *res = db.query(images_tbl);
    if (res){
        delete res;
    } else {
        LFATAL("Could not create images table");
        ret = false;
    }

    //update the video table
    if (ret){
        res = db.query(video_alter);
        if (res){
            delete res;
        } else {
            LFATAL("Could not alter video table");
            ret = false;
        }
    }

    if (ret){
        res = db.query(video_update);
        if (res){
            delete res;
        } else {
            LFATAL("Could not update video table");
            ret = false;
        }
    }

    if (ret){
        return upgrade_from_1_2();
    } else {
        return ret;
    }

}


bool DatabaseManager::upgrade_from_1_2(void){
    const std::string video_alter = "ALTER TABLE `videos` ADD "
        "`codec` varchar(255) COLLATE utf8mb4_unicode_ci DEFAULT NULL AFTER `end_byte`, "
        "ADD `av_type` ENUM('audio', 'video', 'audiovideo') CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT 'audiovideo' AFTER `codec`, "
        "ADD `width` INT NULL DEFAULT NULL AFTER `av_type`, "
        "ADD `height` INT NULL DEFAULT NULL AFTER `width`, "
        "ADD `framerate` FLOAT NULL DEFAULT NULL AFTER `height`";

    bool ret = true;
    DatabaseResult *res = db.query(video_alter);
    if (res){
        delete res;
    } else {
        LFATAL("Could not alter video table");
        ret = false;
    }

    if (ret){
        return upgrade_from_1_3();
    } else {
        return ret;
    }

}

bool DatabaseManager::upgrade_from_1_3(void){
    const std::string events_tbl ="CREATE TABLE `events` ("
        "`id` int(11) NOT NULL AUTO_INCREMENT, "
        "`camera` int(11) NOT NULL, "
        "`img` int(11) DEFAULT NULL, "
        "`source` varchar(64) NOT NULL, "
        "`starttime` bigint(20) NOT NULL, "
        "`x0` int(11) NOT NULL, "
        "`y0` int(11) NOT NULL, "
        "`x1` int(11) NOT NULL, "
        "`y1` int(11) NOT NULL, "
        "`score` float NOT NULL, "
        "PRIMARY KEY (`id`), "
        "KEY camera (`camera`, `source`, `starttime`) "
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci ";

    bool ret = true;
    DatabaseResult *res = db.query(events_tbl);
    if (res){
        delete res;
    } else {
        LFATAL("Could not create events table");
        ret = false;
    }

    if (ret){
        return set_latest_version();
    } else {
        return ret;
    }

}
