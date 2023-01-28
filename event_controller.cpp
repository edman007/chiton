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

#include "event_console.hpp"
#include "event_db.hpp"

EventController::EventController(Config &cfg, Database &db, ImageUtil &img) : ModuleController<EventNotification, EventController>(cfg, db, "event"), img(img) {
    register_module(new ModuleFactory<EventDB, EventNotification, EventController>());
    register_module(new ModuleFactory<EventConsole, EventNotification, EventController>());
    add_mods();
};

EventController::~EventController(){

}

ImageUtil& EventController::get_img_util(void){
    return img;
}

Event& EventController::get_new_event(void){
    for (auto &e : events){
        if (!e.is_valid()){
            e.clear();
            return e;
        }
    }
    events.emplace_back(Event(cfg));
    return events.back();
}

bool EventController::send_event(Event& event){
    bool ret = true;
    for (auto &n : mods){
        ret &= n->send_event(event);
    }
    event.invalidate();
    return ret;
}
