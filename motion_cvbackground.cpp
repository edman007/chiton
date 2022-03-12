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

MotionCVBackground::MotionCVBackground(Config &cfg, Database &db, MotionController &controller) : MotionAlgo(cfg, db, controller, algo_name) {
    ocv = NULL;
    tau = cfg.get_value_double("motion-cvbackground-tau");
    if (tau < 0 || tau > 1){
        tau = 0.01;
    }
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

    //always do the math in CV_16U for extra precision
    if (avg.empty()){
        ocv->get_UMat().convertTo(avg, CV_16U, 256.0);
        ocv->get_UMat().copyTo(low_res);
    } else {
        //compute the frame average
        cv::UMat buf16;
        ocv->get_UMat().convertTo(buf16, CV_16U, 256.0);
        cv::addWeighted(avg, 1-tau, buf16, tau, 0, avg);
        avg.convertTo(low_res, CV_8U, 1.0/256.0);
    }
    return true;
}

bool MotionCVBackground::set_video_stream(const AVStream *stream, const AVCodecContext *codec) {
    return true;
}

const std::string& MotionCVBackground::get_mod_name(void){
    return algo_name;
}

bool MotionCVBackground::init(void) {
    ocv = static_cast<MotionOpenCV*>(controller.get_module_before("opencv", this));
    return true;
}

const cv::UMat MotionCVBackground::get_background(void){
    return low_res;
}

//HAVE_OPENCV
#endif
