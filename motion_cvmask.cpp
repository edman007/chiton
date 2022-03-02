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

#include "motion_cvmask.hpp"
#ifdef HAVE_OPENCV
#include "util.hpp"

#include <opencv2/imgproc.hpp>

static const std::string algo_name = "cvmask";

MotionCVMask::MotionCVMask(Config &cfg, Database &db, MotionController &controller) : MotionAlgo(cfg, db, controller) {
    ocv = NULL;
    background = NULL;
    //FIXME: Make config parameters
    tau = 0.01;
    beta = 3.0;
    thresh = 70;
}

MotionCVMask::~MotionCVMask(){
    ocv = NULL;//we don't own it
    background = NULL;//we don't own it
}

bool MotionCVMask::process_frame(const AVFrame *frame, bool video){
    if (!video){
        return true;
    }
    if (!ocv || !background){
        return false;
    }
    cv::UMat diff;
    cv::absdiff(ocv->get_UMat(), background->get_background(), diff);

    if (sensitivity.empty()){
        diff.copyTo(sensitivity);
    } else {
        cv::addWeighted(sensitivity, 1-tau, diff, beta*tau, 0, sensitivity);
    }
    cv::subtract(diff, sensitivity, masked);
    cv::threshold(masked, masked, thresh, 255, cv::THRESH_BINARY);
    return true;
}

bool MotionCVMask::set_video_stream(const AVStream *stream, const AVCodecContext *codec) {
    return true;
}

const std::string& MotionCVMask::get_name(void) {
    return algo_name;
}

const std::string& MotionCVMaskAllocator::get_name(void) {
    return algo_name;
}

bool MotionCVMask::init(void) {
    ocv = static_cast<MotionOpenCV*>(controller.get_algo_before("opencv", this));
    background = static_cast<MotionCVBackground*>(controller.get_algo_before("cvbackground", this));
    return true;
}

const cv::UMat MotionCVMask::get_masked(void){
    return masked;
}
//HAVE_OPENCV
#endif