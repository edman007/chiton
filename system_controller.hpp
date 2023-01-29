#ifndef __SYSTEM_CONTROLLER_HPP__
#define __SYSTEM_CONTROLLER_HPP__
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
#include "main.hpp"
#include "util.hpp"
//#include "chiton_config.hpp"
#include "mariadb.hpp"
#include "camera.hpp"
#include <unistd.h>
#include <thread>
#include <stdlib.h>
//#include "chiton_ffmpeg.hpp"
#include "database_manager.hpp"
#include "remote.hpp"
#include "export.hpp"

/*
 * System Exit Codes
 */

const int SYS_EXIT_SUCCESS = 0;
const int SYS_EXIT_CFG_ERROR = 1;
const int SYS_EXIT_DB_ERROR = 2;
const int SYS_EXIT_DB_UPDATE_ERROR = 3;
const int SYS_EXIT_SIG_SHUTDOWN = 4;
const int SYS_EXIT_PRIVS_ERROR = 5;

class SystemController {
public:
    //args: initial args to initilize from, cfg-path must be set
    SystemController(Config &args);
    SystemController(const SystemController&) = delete;
    ~SystemController();
    int run(void);//run the main program

    void request_exit(void);
    void request_reload(void);

    //getters
    Config& get_sys_cfg(void);
    Database& get_db(void);
    Export& get_export(void);
    Remote& get_remote(void);
    FileManager& get_fm(void);
private:
    Config &system_args;
    Config cfg;
    std::unique_ptr<Database> db;
    FileManager fm;
    Export expt;
    Remote remote;

    std::atomic_bool exit_requested;
    std::atomic_bool reload_requested;

    std::vector<Camera*> cams;
    std::vector<std::thread> threads;
    char timezone_env[256];//if timezone is changed from default, we need to store it in memory for putenv()

    int run_instance(void);//run an instance of chiton (this exits upon reload)
    void launch_cams(void);//launch all configured cameras
    void loop(void);//execute the main loop
    void load_sys_cfg(Config &cfg);

};
#endif
