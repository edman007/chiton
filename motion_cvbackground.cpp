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

#include "motion_cvbackground.hpp"
#ifdef HAVE_OPENCV
#include "util.hpp"

#include <opencv2/imgproc.hpp>

static const std::string algo_name = "cvbackground";

MotionCVBackground::MotionCVBackground(Config &cfg, Database &db, MotionController &controller) : MotionAlgo(cfg, db, controller) {
    ocv = NULL;
    tau = 0.01;
}

MotionCVBackground::~MotionCVBackground(){
    ocv = NULL;//we don't own it
}

bool MotionCVBackground::process_frame(const AVFrame *frame, bool video){
    if (!video){
        return true;
    }
    if (!ocv){
        return false;
    }
    if (avg.empty()){
        LWARN("Copying MAT");
        ocv->get_UMat().copyTo(avg);
    } else {
        //compute the frame average
        cv::addWeighted(avg, tau, ocv->get_UMat(), 1-tau, 0, avg);
    }
    return true;
}

bool MotionCVBackground::set_video_stream(const AVStream *stream, const AVCodecContext *codec) {
    return true;
}

const std::string& MotionCVBackground::get_name(void) {
    return algo_name;
}

const std::string& MotionCVBackgroundAllocator::get_name(void) {
    return algo_name;
}

bool MotionCVBackground::init(void) {
    ocv = static_cast<MotionOpenCV*>(controller.get_algo_before("opencv", this));
    return true;
}

//HAVE_OPENCV
#endif
