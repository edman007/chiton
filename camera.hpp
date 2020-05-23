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
#include "file_manager.hpp"

class Camera {

    /*
     * This monitors the Recording process for one camera
     */
    
public:
    Camera(int camera, Database& db);
    ~Camera();
    void run(void);//connect and run the camera monitor
    void stop(void);//requests the thread stops
    int ping(void);//checks that this is running, will always return within 1 second, returns 0 if thread is not stalled

private:

    void load_cfg(void);
    int id; //camera ID
    Config cfg;
    Database& db;
    StreamUnwrap stream;
    FileManager fm;
};
#endif
