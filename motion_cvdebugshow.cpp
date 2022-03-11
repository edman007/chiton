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

#include "motion_cvdebugshow.hpp"
#ifdef HAVE_OPENCV
#ifdef DEBUG
#include "util.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

static const std::string algo_name = "cvdebugshow";

MotionCVDebugShow::MotionCVDebugShow(Config &cfg, Database &db, MotionController &controller) : MotionAlgo(cfg, db, controller, algo_name) {
    cvmask = NULL;
    cvbackground = NULL;
    ocv = NULL;
    cvdetect = NULL;
}

MotionCVDebugShow::~MotionCVDebugShow(){
    cvmask = NULL;//we don't own it
    cvbackground = NULL;
    ocv = NULL;
    cvdetect = NULL;
}

bool MotionCVDebugShow::process_frame(const AVFrame *frame, bool video){
    if (!video){
        return true;
    }
    //return true;
    cv::imshow("CVDebugShow - Detect", cvdetect->get_debug_view());
    //cv::imshow("CVDebugShow - Mask", cvmask->get_masked());
    //cv::imshow("CVDebugShow", cvbackground->get_background());
    //cv::imshow("CVDebugShow", ocv->get_UMat());
    cv::waitKey(1);
    return true;
}

bool MotionCVDebugShow::set_video_stream(const AVStream *stream, const AVCodecContext *codec) {
    return true;
}

const std::string& MotionCVDebugShow::get_mod_name(void) {
    return algo_name;
}

bool MotionCVDebugShow::init(void) {
    ocv = static_cast<MotionOpenCV*>(controller.get_module_before("opencv", this));
    cvmask = static_cast<MotionCVMask*>(controller.get_module_before("cvmask", this));
    cvbackground = static_cast<MotionCVBackground*>(controller.get_module_before("cvbackground", this));
    cvdetect = static_cast<MotionCVDetect*>(controller.get_module_before("cvdetect", this));
    return true;
}

//HAVE_OPENCV
#endif
#endif
