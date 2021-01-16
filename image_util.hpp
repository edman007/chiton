#ifndef __IMAGE_UTIL_HPP__
#define __IMAGE_UTIL_HPP__
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

#include "chiton_config.hpp"
#include "chiton_ffmpeg.hpp"
#include "database.hpp"

class ImageUtil {

public:
    ImageUtil(Database &db, Config &cfg);
    ~ImageUtil();

    //write out the source coordinates as a jpeg
    bool write_frame_jpg(const AVFrame *frame, rect src = {-1, -1, 0, 0});
    bool write_frame_png(const AVFrame *frame, rect src = {-1, -1, 0, 0});

private:
    Database &db;
    Config &cfg;

    //Returns a clone of the frame with the new rect applied
    AVFrame* apply_rect(const AVFrame *frame, rect &src);
};

#endif
