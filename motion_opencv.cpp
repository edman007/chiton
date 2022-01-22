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
#include "motion_opencv.hpp"
#include "util.hpp"

static const std::string algo_name = "opencv";

MotionOpenCV::MotionOpenCV(Config &cfg, Database &db) : MotionAlgo(cfg, db) {

}

MotionOpenCV::~MotionOpenCV(){

}
bool MotionOpenCV::process_frame(const AVFrame *frame, bool video){
    LWARN("OpenCV is processing frame");
    return true;
}

bool MotionOpenCV::set_video_stream(const AVStream *stream) {
    LWARN("SET VIDEO Stream, open CV!");
    return true;
}

const std::string& MotionOpenCV::get_name(void) {
    return algo_name;
}

const std::string& MotionOpenCVAllocator::get_name(void) {
    return algo_name;
}
