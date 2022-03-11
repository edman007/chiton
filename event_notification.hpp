#ifndef __EVENT_NOTIFICATION_HPP__
#define __EVENT_NOTIFICATION_HPP__
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

#include "config_build.hpp"
#include "chiton_ffmpeg.hpp"
#include "chiton_config.hpp"

class EventNotification;
#include "event_controller.hpp"

//this class runs all motion detection algorithms
class EventNotification  : public Module<EventNotification, EventController> {
public:
    EventNotification(Config &cfg, Database &db, EventController &controller) : Module<EventNotification, EventController>(cfg, db, controller, "event") {};
    virtual ~EventNotification() {};
    virtual bool send_event(Event &e) = 0;//Send the event through notification method
    virtual const std::string& get_name(void) = 0;//return the name of the notification algorithm
    virtual bool init(void) {return true;};//called immeditly after the constructor to allow dependicies to be setup
};

#endif
