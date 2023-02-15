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
#include "main.hpp"
#include "system_controller.hpp"
#include "util.hpp"
#include "chiton_config.hpp"
#include <unistd.h>
#include <stdlib.h>

#include <csignal>
#include <sys/types.h>
#include <pwd.h>
#include <fstream>
#include <iostream>


static int exit_count = 0;//multiple exit requests will kill the application with this
static SystemController* global_sys_controller = NULL;

void shutdown_signal(int sig){
    if (global_sys_controller){
        global_sys_controller->request_exit();
    }
    exit_count++;
    if (exit_count > 2){
        std::exit(SYS_EXIT_SIG_SHUTDOWN);
    }
}

void reload_signal(int sig){
    if (global_sys_controller){
        global_sys_controller->request_reload();
    }
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
    if (chdir("/")){
        LWARN("chdir / failed");
    }
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
    arg_cfg.set_value("log-color-enabled", "0");//CLI option is disabled by default, against the default setting
    arg_cfg.set_value("verbosity", "");//we set it to this to suppress the default value, the default is taken after loading the cfg

    char options[] = "c:vVdqsp:fP:l";//update man/chiton.1 if you touch this!
    int opt;
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
            case 'l':
                arg_cfg.set_value("log-color-enabled", "1");
                Util::enable_color();
                break;
            }
    }
}

int main (int argc, char **argv){
    Config args;
    process_args(args, argc, argv);

    if (args.get_value("privs-user") != ""){
        if (!drop_privs(args.get_value("privs-user"))){
            return SYS_EXIT_PRIVS_ERROR;//exit
        }
    }

    if (args.get_value_int("fork")){
        if (!fork_background()){
            return SYS_EXIT_SUCCESS;//exit!
        }
    }

    if (args.get_value("pid-file") != ""){
        write_pid(args.get_value("pid-file"));
    }

    if (args.get_value("verbosity") != ""){
        Util::set_log_level(args.get_value_int("verbosity"));
    }
    LWARN("Starting Chiton...");
    LWARN(std::string("\tVersion ") + BUILD_VERSION);
    LWARN(std::string("\tBuilt ") + BUILD_DATE);
    Util::set_thread_name(-1, args);
    gcff_util.load_ffmpeg();
    //load the signal handlers
    std::signal(SIGINT, shutdown_signal);
    std::signal(SIGHUP, reload_signal);

    SystemController controller(args);
    global_sys_controller = &controller;
    int ret = controller.run();

    gcff_util.free_hw();
    Util::disable_syslog();//does nothing if it's not enabled
    Util::disable_color();
    return ret;
}
