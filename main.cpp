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
#include <main.hpp>
#include "util.hpp"
#include "chiton_config.hpp"
#include "mariadb.hpp"
#include "camera.hpp"
#include <unistd.h>
#include <thread>
#include <stdlib.h>
#include "chiton_ffmpeg.hpp"

#include <csignal>
#include <atomic>
#include <chrono>

static char timezone_env[256];//if timezone is changed from default, we need to store it in memory for putenv()
static std::atomic_bool exit_requested(false);
static int exit_count = 0;//multiple exit requests will kill the application with this

static std::atomic_bool reload_requested(false);
void shutdown_signal(int sig){
    exit_requested = true;
    exit_count++;
    if (exit_count > 2){
        std::exit(sig);
    }
}

void reload_signal(int sig){
    reload_requested = true;
}

void load_sys_cfg(Config &cfg) {
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

void run(void){
    Config cfg;
    cfg.load_default_config();
    MariaDB db;
    db.connect(cfg.get_value("db-host"), cfg.get_value("db-database"), cfg.get_value("db-user"), cfg.get_value("db-password"),
               cfg.get_value_int("db-port"), cfg.get_value("db-socket"));

    //load the default config from the database
    DatabaseResult *res = db.query("SELECT name, value FROM config WHERE camera = -1");
    while (res && res->next_row()){
        cfg.set_value(res->get_field(0), res->get_field(1));
    }
    delete res;

    
    //load system config
    load_sys_cfg(cfg);


    FileManager fm(db, cfg);
    //Launch all cameras
    res = db.query("SELECT camera FROM config WHERE camera != -1 AND name = 'active' AND value = '1' GROUP BY camera");
    std::vector<Camera*> cams;
    std::vector<std::thread> threads;
    while (res && res->next_row()){
        LINFO("Loading camera " + std::to_string(res->get_field_long(0)));
        cams.emplace_back(new Camera(res->get_field_long(0), db));//create camera
        threads.emplace_back(&Camera::run, cams.back());//start it
        cams.back()->set_thread_id(threads.back().get_id());

    }
    delete res;

    
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
                            LINFO("Old camera is " + std::to_string((unsigned long)c));
                            c = new Camera(id, db);
                            LINFO("New camera is " + std::to_string((unsigned long)c));
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
        std::this_thread::sleep_for(std::chrono::seconds(10));
    } while (!exit_requested && !reload_requested);

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


}

int main (int argc, char **argv){
    Util::log_msg(LOG_INFO, "Starting Chiton...");
    Util::log_msg(LOG_INFO, std::string("\tVersion ") + GIT_VER);
    Util::log_msg(LOG_INFO, std::string("\tBuilt ") + BUILD_DATE);
    load_ffmpeg();
    //load the signal handlers
    std::signal(SIGINT, shutdown_signal);
    std::signal(SIGHUP, reload_signal);
    
    while (!exit_requested){
        reload_requested = false;
        run();
    }

    return 0;
}





