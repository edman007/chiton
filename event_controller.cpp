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

#include "event_controller.hpp"
#include "util.hpp"


EventController::EventController(Config &cfg, Database &db) : cfg(cfg), db(db) {

};

EventController::~EventController(){

}


Event& EventController::get_new_event(void){
    for (auto &e : events){
        if (!e.is_valid()){
            e.clear();
            return e;
        }
    }
    events.emplace_back(Event(cfg, db));
    return events.back();
}

bool EventController::send_event(Event& event){
    LINFO("Event Sent");
    event.invalidate();
}
