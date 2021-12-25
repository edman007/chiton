#ifndef __IO_WRAPPER_HPP__
#define __IO_WRAPPER_HPP__
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
 *   Copyright 2021 Ed Martin <edman007@edman007.com>
 *
 **************************************************************************
 */

#include "chiton_config.hpp"
#include "chiton_ffmpeg.hpp"

/*
 * Virtual class for Alternate I/O streams
 *
 * This class just allows the StreamUnwrap class to get an AVIOcontext that is user defined
 *
 */

class IOWrapper {
public:
    virtual ~IOWrapper() {};

    virtual AVIOContext* get_pb(void) { return NULL;};//return the AVIOContext
    virtual std::string get_url(void) { return "IOWrapper://";};//return the URL identifying this object
};
#endif
