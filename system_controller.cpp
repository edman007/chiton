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

    //shutdown all cams
    for (auto c : cams){
        c->stop();
    }

    for (auto &t : threads){
        t.join();
    }

    //destruct everything
    while (!cams.empty()){
        delete cams.back();
        cams.pop_back();
    }
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
        LINFO("Loading camera " + std::to_string(res->get_field_long(0)));
        cams.emplace_back(new Camera(*this, res->get_field_long(0)));//create camera
        threads.emplace_back(&Camera::run, cams.back());//start it
        cams.back()->set_thread_id(threads.back().get_id());

    }
    delete res;

    //delete broken segments
    fm.delete_broken_segments();

}

void SystemController::loop(){
    //camera maintance
    do {
        //we should check if they are running and restart anything that froze
        for (auto &c : cams){
            if (c->ping()){
                if (!c->in_startup()){
                    int id = c->get_id();
                    LWARN("Lost connection to cam " + std::to_string(id) + ", restarting...");

                    //then this needs to be restarted
                    c->stop();
                    //find the thread and replace it...
                    for (auto &t : threads){
                        if (t.get_id() == c->get_thread_id()){
                            t.join();
                            delete c;
                            c = new Camera(*this, id);
                            t = std::thread(&Camera::run, c);
                            c->set_thread_id(t.get_id());
                            break;
                        }
                    }
                } else {
                    LWARN("Camera stalled, but appears to be in startup");
                }
            }
        }
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
