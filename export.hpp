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
#include <thread>
#include <atomic>

/*
 * This checks for tasks to export and exports them
 */
class Export {
public:
    Export(Database &db, Config &cfg);
    ~Export(void);

    bool check_for_jobs(void);//checks for any pending export jobs and executes them, returns true if a job is in progress

private:
    Database &db;
    Config &cfg;

    //details of current job:
    long id;
    long starttime;
    long endtime;
    long camera;
    std::string path;
    long progress;

    std::thread runner;
    std::atomic<bool> force_exit;

    bool start_job(void);//kicks off a thread to perform the export
    void run_job(void);//main loop for exporting
};

#endif
