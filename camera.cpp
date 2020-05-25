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

#include "camera.hpp"
#include "util.hpp"
#include "stream_writer.hpp"

Camera::Camera(int camera, Database& db) : id(camera), db(db), stream(cfg), fm(db, cfg) {
    //load the config
    load_cfg();
    //stream = StreamUnwrap(cfg);
    shutdown = false;
    alive = true;
    watchdog = false;
}
Camera::~Camera(){

}

void Camera::load_cfg(void){
    //loads the global and then overwrites it with the local
    DatabaseResult *res = db.query("SELECT name, value FROM config WHERE camera IS NULL OR camera = " + std::to_string(id) + " ORDER by camera DESC" );
    while (res && res->next_row()){
        cfg.set_value(res->get_field(0), res->get_field(1));
    }
    delete res;
}
    
void Camera::run(void){
    LINFO("Camera " + std::to_string(id) + " starting...");
    if (!stream.connect()){
        alive = false;
        return;
    }
    LINFO("Camera " + std::to_string(id) + " connected...");
    long int file_id;
    struct timeval start;
    Util::get_videotime(start);
    std::string new_output = fm.get_next_path(file_id, id, start);

    StreamWriter out = StreamWriter(cfg, new_output, stream);
    out.open();

    AVPacket pkt;
    bool valid_keyframe = false;
    long max_dts = 0;
    while (!shutdown && stream.get_next_frame(pkt)){
        watchdog = true;
        if (valid_keyframe || pkt.flags & AV_PKT_FLAG_KEY){
            out.write(pkt);//log it
            valid_keyframe = true;
            //LINFO("Got Frame " + std::to_string(id));
        } else {
            LINFO("Waiting for a keyframe...");
        }
        
        stream.unref_frame(pkt);
    }
    LINFO("Camera " + std::to_string(id)+ " is exiting");
    out.close();
    alive = false;
}

void Camera::stop(void){
    shutdown = true;
}

bool Camera::ping(void){
    return !watchdog.exchange(false) || !alive;
}

int Camera::get_id(void){
    return id;
}
