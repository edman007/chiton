#ifndef __CAMERA_STATUS_HPP__
#define __CAMERA_STATUS_HPP__
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
#include "chiton_config.hpp"
#include "util.hpp"
#include <atomic>
/*
 * Helper class to track camera running status
 */


class CameraStatus {
public:
    CameraStatus(int id);
    ~CameraStatus();
    CameraStatus(const CameraStatus &in);
    CameraStatus& operator=(const CameraStatus &in);//copy
    CameraStatus& operator+=(const CameraStatus &in);//add

    void add_bytes_written(unsigned int bytes);//add bytes to running write total
    void add_bytes_read(unsigned int bytes);//add bytes to running read total
    void add_dropped_frame(void);//increment dropped frame counter
    void set_start_time(time_t start);

    int get_id(void);
    unsigned int get_bytes_written(void);
    unsigned int get_bytes_read(void);
    unsigned int get_dropped_frames(void);
    time_t get_start_time(void);
private:
    std::atomic<int> id;
    std::atomic<unsigned int> bytes_written;
    std::atomic<unsigned int> bytes_read;
    std::atomic<unsigned int> dropped_frames;
    std::atomic<time_t> start_time;

};


#endif
