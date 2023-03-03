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
 *   Copyright 2023 Ed Martin <edman007@edman007.com>
 *
 **************************************************************************
 */

#include "camera_status.hpp"

CameraStatus::CameraStatus(int id) : id(id) {
    bytes_written = 0;
    bytes_read = 0;
    dropped_frames = 0;
    start_time = 0;
}

CameraStatus::CameraStatus(const CameraStatus &in){
    *this = in;//same as copy operator
}

CameraStatus& CameraStatus::operator=(const CameraStatus &in){
    //everything in here is atomics, so this is safe, as this is typically run while in may be modified
    //100% synchronizing isn't needed here
    id = in.id.load(std::memory_order_relaxed);
    bytes_written = in.bytes_written.load(std::memory_order_relaxed);
    bytes_read = in.bytes_read.load(std::memory_order_relaxed);
    dropped_frames = in.dropped_frames.load(std::memory_order_relaxed);
    start_time = in.start_time.load(std::memory_order_relaxed);
    return *this;
}

CameraStatus& CameraStatus::operator+=(const CameraStatus &in){
    //everything in here is atomics, so this is safe, as this is typically run while in may be modified
    //100% synchronizing isn't needed here
    //id; //id is not copied

    //counters are added
    bytes_written.fetch_add(in.bytes_written.load(std::memory_order_relaxed), std::memory_order_relaxed);
    bytes_read.fetch_add(in.bytes_read.load(std::memory_order_relaxed), std::memory_order_relaxed);
    dropped_frames.fetch_add(in.dropped_frames.load(std::memory_order_relaxed), std::memory_order_relaxed);

    //unuiqe are min/max'd
    auto old_start = start_time.load(std::memory_order_relaxed);
    bool retry = false;
    do {
        auto in_start = in.start_time.load(std::memory_order_relaxed);
        if (in_start < old_start){
            retry = !start_time.compare_exchange_weak(old_start, in_start,  std::memory_order_release,  std::memory_order_relaxed);
        }
    } while (retry);
    return *this;

}

CameraStatus::~CameraStatus(){
    //nothing
}

void CameraStatus::add_bytes_written(unsigned int bytes){
    bytes_written += bytes;
}

void CameraStatus::add_bytes_read(unsigned int bytes){
    bytes_read += bytes;
}

void CameraStatus::add_dropped_frame(void){
    dropped_frames++;
}

unsigned int CameraStatus::get_bytes_written(void){
    return bytes_written;
}

unsigned int CameraStatus::get_bytes_read(void){
    return bytes_read;
}

unsigned int CameraStatus::get_dropped_frames(void){
    return dropped_frames;
}

void CameraStatus::set_start_time(time_t start){
    start_time = start;
}

time_t CameraStatus::get_start_time(void){
    return start_time;
}

int CameraStatus::get_id(void){
    return id;
}
