#ifndef __EVENT_DB_HPP__
#define __EVENT_DB_HPP__
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

#include "event_notification.hpp"
#include "image_util.hpp"

//this writes events to the db/logging
class EventDB : public EventNotification {
public:
    EventDB(Config &cfg, Database &db, EventController &controller);
    ~EventDB();
    bool send_event(Event &e);//Send the event through notification method
    static const std::string& get_mod_name(void);
private:

};
#endif
