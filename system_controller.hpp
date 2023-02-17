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
#include <list>
#include <set>
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

/*
 * SystemController:
 * - This class manages all cameras, starting and stopping them
 * - This class also manages access between cameras, the remote, and system functions
 */
class SystemController {
public:
    enum CAMERA_STATUS {
        STOPPING = 0,
        STARTING,
        RUNNING,
        RESTARTING
    };
    //args: initial args to initilize from, cfg-path must be set
    SystemController(Config &args);
    SystemController(const SystemController&) = delete;
    ~SystemController();
    int run(void);//run the main program

    void request_exit(void);//request that the program exits
    void request_reload(void);//requests that all cameras are restarted

    bool stop_cam(int id);//stop a given camera
    bool start_cam(int id);//start the cam with given ID
    bool restart_cam(int id);//restart tha cam with a given ID

    void list_status(std::map<int, CAMERA_STATUS> &stat);//writes to the map the status of all cameras

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

    std::list<Camera> cams;//all the currently running cameras
    std::mutex cams_lock;//to manage access to the cameras, never lock after locking cams_set_lock, can only be locked before

    //these lists are used to request camera's are stop/restarted and can be set from other threads
    std::set<int> stop_cams_list;//cams stopped that need to be joined
    std::set<int> start_cams_list;//list of cameras to start
    std::set<int> startup_cams_list;//list of cameras currently in startup
    std::mutex cam_set_lock;//lock to manage the two above sets

    char timezone_env[256];//if timezone is changed from default, we need to store it in memory for putenv()

    int run_instance(void);//run an instance of chiton (this exits upon reload)
    void launch_cams(void);//launch all configured cameras
    void loop(void);//execute the main loop
    void load_sys_cfg(Config &cfg);

    void stop_cams(void);//stop and join all required cams
    void start_cams(void);//start all required cams

};
#endif
