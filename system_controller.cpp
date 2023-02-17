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
 *   Copyright 2023 Ed Martin <edman007@edman007.com>
 *
 **************************************************************************
 */


#include "system_controller.hpp"
#include "util.hpp"
#include "mariadb.hpp"
#include "camera.hpp"
#include <thread>
#include <stdlib.h>
#include "chiton_ffmpeg.hpp"
#include "database_manager.hpp"
#include "remote.hpp"
#include "export.hpp"
#include <atomic>
#include <chrono>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <functional>

SystemController::SystemController(Config &args) :
    system_args(args),
    db(std::unique_ptr<Database>(new MariaDB())),
    fm(*this),
    expt(*this),
    remote(*this),
    exit_requested(false),
    reload_requested(false){


}

SystemController::~SystemController(){
}

int SystemController::run(void){
    int ret = 0;
    while (!exit_requested && !ret){
        reload_requested = false;
        ret = run_instance();
    }
    return ret;
}

int SystemController::run_instance(void){
    if (!cfg.load_config(system_args.get_value("cfg-path"))){
        exit_requested = true;//this is a fatal error
        return SYS_EXIT_CFG_ERROR;
    }

    //set the correct verbosity if it is not supplied on the command line
    if (system_args.get_value("verbosity") == "" && cfg.get_value("verbosity") != ""){
        Util::set_log_level(cfg.get_value_int("verbosity"));
    }

    //set the history length
    Util::set_history_len(cfg.get_value_int("log-history-len"));

    if (db->connect(cfg.get_value("db-host"), cfg.get_value("db-database"), cfg.get_value("db-user"), cfg.get_value("db-password"),
                   cfg.get_value_int("db-port"), cfg.get_value("db-socket"))){
        LFATAL("Could not connect to the database! Check your configuration");
        exit_requested = true;
        return SYS_EXIT_DB_ERROR;
    }

    DatabaseManager dbm(*db);
    if (!dbm.check_database()){
        //this is fatal, cannot get the database into a usable state
        exit_requested = true;
        return SYS_EXIT_DB_UPDATE_ERROR;
    }

    launch_cams();//launch all configured cameras
    loop();//main loop
    remote.shutdown();
    cams_lock.lock();
    //shutdown all cams
    for (auto &c : cams){
        c.stop();
    }

    for (auto &c : cams){
        c.join();
    }

    //destruct everything
    cams.clear();
    cam_set_lock.lock();
    stop_cams_list.clear();
    start_cams_list.clear();
    cam_set_lock.unlock();
    cams_lock.unlock();
    db->close();
    return SYS_EXIT_SUCCESS;
}

Config& SystemController::get_sys_cfg(void){
    return cfg;
}

Database& SystemController::get_db(void){
    return *db;
}

Export& SystemController::get_export(void){
    return expt;
}

Remote& SystemController::get_remote(void){
    return remote;
}

FileManager& SystemController::get_fm(void){
    return fm;
}


void SystemController::launch_cams(void){
    //load the default config from the database
    DatabaseResult *res = db->query("SELECT name, value FROM config WHERE camera = -1");
    while (res && res->next_row()){
        cfg.set_value(res->get_field(0), res->get_field(1));
    }
    delete res;

    //load system config
    load_sys_cfg(cfg);
    Util::load_colors(cfg);
    if (cfg.get_value_int("log-color-enabled") || system_args.get_value_int("log-color-enabled") ){
        Util::enable_color();
    } else {
        Util::disable_color();
    }

    fm.init();
    remote.init();

    //Launch all cameras
    res = db->query("SELECT camera FROM config WHERE camera != -1 AND name = 'active' AND value = '1' GROUP BY camera");
    while (res && res->next_row()){
        start_cam(res->get_field_long(0));//delayed start
    }
    delete res;

    //delete broken segments
    fm.delete_broken_segments();

}

void SystemController::loop(){
    //camera maintance
    do {
        //we should check if they are running and restart anything that froze
        cams_lock.lock();

        //clear the list of cameras in startup
        cam_set_lock.lock();
        startup_cams_list.clear();
        cam_set_lock.unlock();
        for (auto &c : cams){
            if (c.ping()){
                int id = c.get_id();
                if (!c.in_startup()){
                    LWARN("Lost connection to cam " + std::to_string(id) + ", restarting...");
                    restart_cam(id);
                } else {
                    LWARN("Camera stalled, but appears to be in startup");
                    cam_set_lock.lock();
                    startup_cams_list.insert(id);
                    cam_set_lock.unlock();
                }
            }
        }
        cam_set_lock.lock();
        stop_cams();
        start_cams();
        cam_set_lock.unlock();
        cams_lock.unlock();
        fm.clean_disk();
        expt.check_for_jobs();
        std::this_thread::sleep_for(std::chrono::seconds(10));
    } while (!exit_requested && !remote.get_reload_request() && !reload_requested);

}

void SystemController::request_exit(void){
    exit_requested = true;
}

void SystemController::request_reload(void){
    reload_requested = true;
}

void SystemController::load_sys_cfg(Config &cfg) {
    std::string timezone = cfg.get_value("timezone");
    if (timezone.compare("")){
        LINFO("Setting Timezone to " + timezone);
        timezone = "TZ=" + timezone;
        auto len = timezone.copy(timezone_env, sizeof timezone_env, 0);
        timezone_env[len] = '\0';
        putenv(timezone_env);
    }
    tzset();

}

bool SystemController::stop_cam(int id){
    cam_set_lock.lock();
    auto p = stop_cams_list.insert(id);
    cam_set_lock.unlock();
    return p.second;
}

bool SystemController::start_cam(int id){
    cam_set_lock.lock();
    auto p = start_cams_list.insert(id);
    cam_set_lock.unlock();
    return p.second;
}

bool SystemController::restart_cam(int id){
    cam_set_lock.lock();
    stop_cams_list.insert(id);
    auto p = start_cams_list.insert(id);
    cam_set_lock.unlock();
    return p.second;
}

void SystemController::stop_cams(void){
    for (auto &stop : stop_cams_list){
        for (auto &c : cams){
            if (c.get_id() == stop){
                c.stop();
                break;
            }
        }
    }
    for (auto &join : stop_cams_list){
        for (auto it = cams.begin(); it != cams.end(); it++){
            auto &c = *it;
            if (c.get_id() == join){
                c.join();
                cams.erase(it);
                break;
            }
        }
    }
    stop_cams_list.clear();
}

void SystemController::start_cams(void){
    for (auto &start : start_cams_list){
        //check if cam is already running and refuse to start a duplicate camera
        bool dup = false;
        for (auto &c : cams){
            if (c.get_id() == start){
                dup = true;
                break;
            }
        }
        if (dup){
            LDEBUG("Cam " + std::to_string(start) + " already started");
            continue;
        }
        DatabaseResult *res = db->query("SELECT camera FROM config WHERE camera = '" + std::to_string(start) +
                                        "' AND name = 'active' AND value = '1' LIMIT 1");
        if (res && res->next_row()){
            LINFO("Loading camera " + std::to_string(res->get_field_long(0)));
            cams.emplace_back(*this, res->get_field_long(0));//create camera
            auto &cam = cams.back();
            cam.start();
        }
        delete res;
    }
    start_cams_list.clear();
}

void SystemController::list_status(std::map<int, CAMERA_STATUS> &stat){
    stat.clear();

    //assign all as running
    cams_lock.lock();
    cam_set_lock.lock();
    for (auto &c : cams){
        stat[c.get_id()] = CAMERA_STATUS::RUNNING;
    }
    cams_lock.unlock();//unlocking here is ok, but can't be relocked withot unlocking cam_set_lock first

    //mark the ones still in startup
    for (auto s : startup_cams_list){
        stat[s] = CAMERA_STATUS::STARTING;
    }

    //mark the ones stopping
    for (auto s : stop_cams_list){
        stat[s] = CAMERA_STATUS::STOPPING;
    }

    //mark the ones starting or restarting
    for (auto s : start_cams_list){
        if (stop_cams_list.find(s) == start_cams_list.end()){
            //not on stop list, so starting
            stat[s] = CAMERA_STATUS::STARTING;
        } else {
            //on stop and start list, restarting
            stat[s] = CAMERA_STATUS::RESTARTING;
        }
    }

    cam_set_lock.unlock();

}
