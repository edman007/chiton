#ifndef __CAMERA_HPP__
#define __CAMERA_HPP__
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

#include "database.hpp"
#include "chiton_config.hpp"
#include "stream_unwrap.hpp"
#include "stream_writer.hpp"
#include "file_manager.hpp"
#include <atomic>
#include <thread>

class Camera {
  
    /*
     * This monitors the Recording process for one camera
     */
    
public:
    Camera(int camera, Database& db, const Config &sys_cfg);
    ~Camera();
    void run(void);//connect and run the camera monitor
    void stop(void);//requests the thread stops
    bool ping(void);//checks that this is running, returns true if the thread has not progressed (processed at least one frame) since last ping
    bool in_startup(void);//returns true if we have not completed connecting
    void set_thread_id(std::thread::id tid);
    std::thread::id get_thread_id(void);
    int get_id(void);//return this camera's ID
private:

    void load_cfg(void);
    int id; //camera ID
    Config cfg;
    Database& db;
    StreamUnwrap stream;
    FileManager fm;
    std::atomic_bool alive;//used by ping to check our status
    std::atomic_bool watchdog;//used by ping to check our status
    std::atomic_bool shutdown;//signals that we should exit
    std::atomic_bool startup;//used to identify if we are in an extended wait due to startup
    std::thread::id thread_id;//used for tracking our thread

    AVPacket *pkt;//packet we are processing
    AVRational last_cut;//last time a segment was cut
    AVRational last_cut_file;//last time a file was cut
    long long last_cut_byte;//the bytes of the end of the previous segment (or file)
    AVRational seconds_per_file;//max seconds per file
    AVRational seconds_per_segment;//max seconds per segment

    long int file_id;//database id of current file we are writing to
    //check if packet is a keyframe and switch the filename as needed
    void cut_video(const AVPacket *pkt, StreamWriter &out);
    bool get_vencode(void);//get if video needs to be encoded
    bool get_aencode(void);//get if audio needs to be encoded
};
#endif
