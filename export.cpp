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
#include "export.hpp"
#include "util.hpp"

Export::Export(Database &db, Config &cfg) : db(db), cfg(cfg) {
    force_exit = false;
}

Export::~Export(void){
    force_exit = true;
    if (runner.joinable()){
        runner.join();
    }
}

bool Export::check_for_jobs(void){
    const std::string sql = "SELECT id, starttime, endtime, camera, path, progress FROM exports WHERE progress != 100 LIMIT 1";
    DatabaseResult *res = db.query(sql);
    if (res && res->next_row()){
        id = res->get_field_long(0);
        starttime = res->get_field_long(1);
        endtime = res->get_field_long(2);
        camera = res->get_field_long(3);
        path = std::string(res->get_field(4));
        progress = res->get_field_long(5);
        delete res;

        return start_job();
    }
    return false;
}

bool Export::start_job(void){
    runner = std::thread(&Export::run_job, this);
    return runner.joinable();
}

void Export::run_job(void){
    Util::set_low_priority();//reduce the thread priority
    LWARN("THREAD RUNNING");
}
