#ifndef __EVENT_CONTROLLER_HPP__
#define __EVENT_CONTROLLER_HPP__
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
#include "module_controller.hpp"
#include "event.hpp"
class EventController;
#include "event_notification.hpp"

//this class manages events, sending out notifications
class EventController : public ModuleController<EventNotification, EventController> {
public:
    EventController(Config &cfg, Database &db);
    ~EventController();
    Event& get_new_event(void);//allocate a new Event, note, Event must be sent via send_event() or marked invalid
    bool send_event(Event& event);//send event, it is not valid to access the Event after calling send_event()
    //returns the event notification algorithm with name, will be executed before algo, returns null if the algorithm does not exist, calls init() on the algorithm
    EventNotification* get_notification_before(const std::string &name, const EventNotification *algo);
    void register_event_notification(ModuleAllocator<EventNotification, EventController>* maa);//registers an allocator and makes an algorithm available for use

private:
    std::vector<Event> events;
};
#endif
