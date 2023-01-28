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

#include "event_db.hpp"


static std::string algo_name = "db";

EventDB::EventDB(Config &cfg, Database &db, EventController &controller) : EventNotification(cfg, db, controller, algo_name) {

}

EventDB::~EventDB(){

}

bool EventDB::send_event(Event &e){
    //save the thumbnail
    long im_id = -1;
    const AVFrame *frame = e.get_frame();
    const struct timeval &e_time = e.get_timestamp();
    const std::vector<float>& pos = e.get_position();
    if (frame){
        rect dims = {
            .x =  static_cast<int>(pos[0]), .y = static_cast<int>(pos[1]),
            .w = static_cast<int>(pos[2] - pos[0]), .h= static_cast<int>(pos[3] - pos[1])
        };
        if (dims.x < 0){
            dims.x = 0;
        }
        LWARN("x: " + std::to_string(dims.x) + " y: " + std::to_string(dims.y) +
            " w: " + std::to_string(dims.w) + " h: " + std::to_string(dims.h));
        std::string source = std::string(e.get_source());
        if (!controller.get_img_util().write_frame_jpg(frame, source, &e_time, dims, &im_id)){
            im_id = -1;
        }
    }
    std::string ptime = std::to_string(Util::pack_time(e_time));
    std::string sql = "INSERT INTO events (camera, img, source, starttime, x0, y0, x1, y1, score) VALUES ("
        + cfg.get_value("camera-id") + "," + std::to_string(im_id) + ",'" + db.escape(e.get_source()) + "'," + ptime + ","
        + std::to_string(pos[0]) + "," + std::to_string(pos[1]) + "," + std::to_string(pos[2]) + "," + std::to_string(pos[3]) + "," +
        std::to_string(e.get_score()) + ")";
    DatabaseResult* res = db.query(sql);
    if (res){
        delete res;
    } else {
        return false;
    }

    return true;
}

const std::string& EventDB::get_mod_name(void){
    return algo_name;
}
