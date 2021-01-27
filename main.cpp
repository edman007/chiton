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
#include "database_manager.hpp"
#include "remote.hpp"
#include "export.hpp"

#include <csignal>
#include <atomic>
#include <chrono>
#include <sys/types.h>
#include <pwd.h>
#include <fstream>
#include <iostream>

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

void run(Config& args){
    Config cfg;
    if (!cfg.load_config(args.get_value("cfg-path"))){
        exit_requested = true;//this is a fatal error
        return;
    }

    //set the correct verbosity if it is not supplied on the command line
    if (args.get_value("verbosity") == "" && cfg.get_value("verbosity") != ""){
        Util::set_log_level(cfg.get_value_int("verbosity"));
    }

    MariaDB db;
    if (db.connect(cfg.get_value("db-host"), cfg.get_value("db-database"), cfg.get_value("db-user"), cfg.get_value("db-password"),
                   cfg.get_value_int("db-port"), cfg.get_value("db-socket"))){
        LFATAL("Could not connect to the database! Check your configuration");
        exit_requested = true;
        return;
    }

    DatabaseManager dbm(db);
    if (!dbm.check_database()){
        //this is fatal, cannot get the database into a usable state
        exit_requested = true;
        return;
    }
    //load the default config from the database
    DatabaseResult *res = db.query("SELECT name, value FROM config WHERE camera = -1");
    while (res && res->next_row()){
        cfg.set_value(res->get_field(0), res->get_field(1));
    }
    delete res;

    
    //load system config
    load_sys_cfg(cfg);
    load_vaapi();

    FileManager fm(db, cfg);
    Export expt(db, cfg, fm);
    Remote remote(db, cfg, expt);

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

    //delete broken segments
    fm.delete_broken_segments();
    
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
        expt.check_for_jobs();
        std::this_thread::sleep_for(std::chrono::seconds(10));
    } while (!exit_requested && !remote.get_reload_request() && !reload_requested);

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

    free_vaapi();

}


//fork to the background (returns false if fork failed or this is the parent process)
bool fork_background(void){
    pid_t pid = fork();
    if (pid < 0){
        LFATAL("Failed to fork");
        return false;
    }
    if (pid != 0){
        return false;
    }
    pid_t sid = setsid();
    if (sid < 0){
        LFATAL("Failed to setsid()");
        return false;
    }
    chdir("/");
    //Close stdin. stdout and stderr
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    return true;
}

bool drop_privs(const std::string& username){
    if (getuid() != 0){
        LFATAL("Cannot drop privleges because you are not root!");
        return false;
    }

    struct passwd* user = getpwnam(username.c_str());
    if (user == NULL){
        LFATAL("Could not find user " + username);
        return false;
    }

    if (setgid(user->pw_gid)){
        LFATAL("Could not switch group " + std::to_string(user->pw_gid) + ", " + strerror(errno));
        return false;
    }

    if (setuid(user->pw_uid)){
        LFATAL("Could not switch user");
        return false;
    }


    return true;
}
//write the pid to a file
bool write_pid(const std::string& path){
    pid_t pid = getpid();
    std::ofstream out;
    out.open(path, std::ofstream::out | std::ofstream::trunc);
    if (out.rdstate() & std::ifstream::failbit){
        LWARN("Could not open pid file: " + path);
    }
    out << pid;
    out.close();
    return true;
}
//this reads the arguments, writes the Config object for received parameters
void process_args(Config& arg_cfg, int argc, char **argv){
    //any system wide defaults...these are build-time defaults
    arg_cfg.set_value("cfg-path", SYSCFGPATH);

    char options[] = "c:vVdqsp:fP:";//update man/chiton.1 if you touch this!
    char opt;
    while ((opt = getopt(argc, argv, options)) != -1){
            switch (opt) {
            case 'c':
                arg_cfg.set_value("cfg-path", optarg);
                break;
            case 'v':
                arg_cfg.set_value("verbosity", "3");
                break;
            case 'V':
                arg_cfg.set_value("verbosity", "4");
                break;
            case 'd':
                arg_cfg.set_value("verbosity", "5");
                break;
            case 'q':
                arg_cfg.set_value("verbosity", "0");
                break;
            case 's':
                Util::enable_syslog();
                break;
            case 'p':
                arg_cfg.set_value("pid-file", optarg);
                break;
            case 'f':
                arg_cfg.set_value("fork", "1");
                Util::enable_syslog();
                break;
            case 'P':
                arg_cfg.set_value("privs-user", optarg);
                break;

            }
    }
}

int main (int argc, char **argv){
    Config args;
    process_args(args, argc, argv);

    if (args.get_value("privs-user") != ""){
        if (!drop_privs(args.get_value("privs-user"))){
            return 1;//exit
        }
    }

    if (args.get_value_int("fork")){
        if (!fork_background()){
            return 0;//exit!
        }
    }

    if (args.get_value("pid-file") != ""){
        write_pid(args.get_value("pid-file"));
    }

    if (args.get_value("verbosity") != ""){
        Util::set_log_level(args.get_value_int("verbosity"));
    }
    LWARN("Starting Chiton...");
    LWARN(std::string("\tVersion ") + GIT_VER);
    LINFO(std::string("\tBuilt ") + BUILD_DATE);
    load_ffmpeg();
    //load the signal handlers
    std::signal(SIGINT, shutdown_signal);
    std::signal(SIGHUP, reload_signal);
    
    while (!exit_requested){
        reload_requested = false;
        run(args);
    }
    Util::disable_syslog();//does nothing if it's not open

    return 0;
}
