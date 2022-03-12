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
 *   Copyright 2022 Ed Martin <edman007@edman007.com>
 *
 **************************************************************************
 */

#include "event_console.hpp"
#include <sstream>
#include <ctime>

static std::string algo_name = "console";

EventConsole::EventConsole(Config &cfg, Database &db, EventController &controller) : EventNotification(cfg, db, controller, algo_name){

}

EventConsole::~EventConsole(){

}

bool EventConsole::send_event(Event &e){
    std::ostringstream s;
    s << "EVENT: " << e.get_source() << " @ \"";
    const struct timeval &etime = e.get_timestamp();
    struct tm real_time;
    localtime_r(&etime.tv_sec, &real_time);
    const int date_len = 64;
    char date_str[date_len];
    size_t ret = strftime(date_str, date_len - 1, "%c", &real_time);
    if (ret > 0){
        s << date_str << "";
    } else {
        s << "??";
    }

    s << "\", position: ";
    const std::vector<float>& pos = e.get_position();
    s << pos[0] << "x" << pos[1] << "-" << pos[2] << "x" << pos[3];
    LINFO(s.str());
    return true;
}

const std::string& EventConsole::get_mod_name(void){
    return algo_name;
}
