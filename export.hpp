#ifndef __EXPORT_HPP__
#define __EXPORT_HPP__
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
#include "chiton_config.hpp"
#include "database.hpp"
#include "file_manager.hpp"
#include <thread>
#include <atomic>
#include <mutex>

/*
 * This checks for tasks to export and exports them
 */
class Export {
public:
    Export(Database &db, Config &cfg, FileManager &fm);
    ~Export(void);

    bool check_for_jobs(void);//checks for any pending export jobs and executes them, returns true if a job is in progress
    bool rm_export(int export_id);//deletes export with this ID, cancles it if in progress, thread safe
private:
    Database &db;
    Config &cfg;
    FileManager &g_fm;


    //details of current job:
    std::atomic<long> id;
    long starttime;
    long endtime;
    long camera;
    std::string path;
    long progress;
    Config* camera_cfg;
    long reserved_bytes;

    std::thread runner;
    std::atomic<bool> export_in_progress;//set to true when the runner is active
    std::atomic<bool> force_exit;//will cause the runner to exis ASAP when true
    std::mutex lock;//lock to check on the runner

    bool start_job(void);//kicks off a thread to perform the export
    void run_job(void);//main loop for exporting
    bool update_progress();//sets the current progress
    void reserve_space(FileManager &fm, long size);//reserve size bytes of drive space
    bool split_export(long seg_id);//split the export so that a new export is created starting with this segment
};

#endif
