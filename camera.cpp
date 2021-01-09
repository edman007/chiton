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
 *   Copyright 2020-2021 Ed Martin <edman007@edman007.com>
 *
 **************************************************************************
 */

#include "camera.hpp"
#include "util.hpp"
#include "stream_writer.hpp"

Camera::Camera(int camera, Database& db) : id(camera), db(db), stream(cfg), fm(db, cfg) {
    //load the config
    load_cfg();
    shutdown = false;
    alive = true;
    watchdog = false;
    startup = true;
}
Camera::~Camera(){
    
}

void Camera::load_cfg(void){
    cfg.load_camera_config(id, db);
}
    
void Camera::run(void){
    LINFO("Camera " + std::to_string(id) + " starting...");
    if (!stream.connect()){
        alive = false;
        startup = false;
        return;
    }
    watchdog = true;;
    startup = false;

    LINFO("Camera " + std::to_string(id) + " connected...");
    long int file_id;
    std::string new_output = fm.get_next_path(file_id, id, stream.get_start_time());

    StreamWriter out = StreamWriter(cfg, new_output, stream);
    out.open();
    
    AVPacket pkt;
    bool valid_keyframe = false;

    //allocate a frame (we only allocate when we want to decode, an unallocated frame means we skip decoding)
    AVFrame *frame = NULL;
    frame = av_frame_alloc();
    if (!frame){
        LWARN("Failed to allocate frame");
        frame = NULL;
    }

    //variables for cutting files...
    AVRational last_cut = av_make_q(0, 1);
    int seconds_per_file_raw = cfg.get_value_int("seconds-per-file");
    if (seconds_per_file_raw <= 0){
        LWARN("seconds-per-file was invalid");
        seconds_per_file_raw = DEFAULT_SECONDS_PER_FILE;
    }
    AVRational seconds_per_file = av_make_q(seconds_per_file_raw, 1);

    //used for calculating shutdown time for the last segment
    long last_pts = 0;
    long last_stream_index = 0;
    while (!shutdown && stream.get_next_frame(pkt)){
        watchdog = true;
        last_pts = pkt.pts;
        last_stream_index = pkt.stream_index;
        
        if (pkt.flags & AV_PKT_FLAG_KEY && stream.get_format_context()->streams[pkt.stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            //calculate the seconds:
            AVRational sec = av_mul_q(av_make_q(pkt.dts, 1), stream.get_format_context()->streams[pkt.stream_index]->time_base);//current time..
            sec = av_sub_q(sec, last_cut);
            if (av_cmp_q(sec, seconds_per_file) == 1 || sec.num < 0){
                //cutting the video
                struct timeval start;
                Util::compute_timestamp(stream.get_start_time(), start, pkt.pts, stream.get_format_context()->streams[pkt.stream_index]->time_base);
                out.close();
                fm.update_file_metadata(file_id, start);
                if (sec.num < 0){
                    //this will cause a discontinuity which will be picked up and cause it to play correctly
                    start.tv_usec += 1000;
                }
                new_output = fm.get_next_path(file_id, id, start);
                out.change_path(new_output);
                if (!out.open()){
                    shutdown = true;
                }
                //save out this position
                last_cut = av_mul_q(av_make_q(pkt.dts, 1), stream.get_format_context()->streams[pkt.stream_index]->time_base);
            }
        }        
        if (valid_keyframe || (pkt.flags & AV_PKT_FLAG_KEY && stream.get_format_context()->streams[pkt.stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)){
            out.write(pkt);//log it
            valid_keyframe = true;
            //LINFO("Got Frame " + std::to_string(id));
        }

        //decode the packets
        if (frame){
            if (stream.decode_packet(pkt)){
                while (stream.get_decoded_frame(pkt.stream_index, frame)){
                    LWARN("Decoded Frame");
                }
            }
        }
        
        stream.unref_frame(pkt);
    }
    LINFO("Camera " + std::to_string(id)+ " is exiting");

    struct timeval end;
    Util::compute_timestamp(stream.get_start_time(), end, last_pts, stream.get_format_context()->streams[last_stream_index]->time_base);
    out.close();
    fm.update_file_metadata(file_id, end);

    if (frame){
        av_frame_free(&frame);
        frame = NULL;
    }

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

bool Camera::in_startup(void){
    return startup && alive;
}


void Camera::set_thread_id(std::thread::id tid){
    thread_id = tid;
}
std::thread::id Camera::get_thread_id(void){
    return thread_id;
}
