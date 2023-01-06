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

#include "motion_cvresize.hpp"
#ifdef HAVE_OPENCV
#include "util.hpp"

#include <opencv2/imgproc.hpp>


static const std::string algo_name = "cvresize";

MotionCVResize::MotionCVResize(Config &cfg, Database &db, MotionController &controller) : MotionAlgo(cfg, db, controller, algo_name) {
    scale_ratio = cfg.get_value_double("motion-cvresize-scale");
    if (scale_ratio <= 0 || scale_ratio > 1){
         scale_ratio = 1;
    }
    ocv = NULL;
 }

MotionCVResize::~MotionCVResize(){
    ocv = NULL;
}

bool MotionCVResize::process_frame(const AVFrame *frame, bool video){
    if (!video || !ocv){
        return true;
    }
    if (scale_ratio == 1){
        return true;
    }
    cv::resize(ocv->get_UMat(), buf_mat, cv::Size(), scale_ratio, scale_ratio, cv::INTER_NEAREST);
    return true;
}

bool MotionCVResize::init(void) {
    ocv = static_cast<MotionOpenCV*>(controller.get_module_before("opencv", this));
    return true;
}

bool MotionCVResize::set_video_stream(const AVStream *stream, const AVCodecContext *codec) {
    return true;
}

const std::string& MotionCVResize::get_mod_name(void) {
    return algo_name;
}

const cv::UMat& MotionCVResize::get_UMat(void) const {
    if (scale_ratio == 1){
        return ocv->get_UMat();
    }
    return buf_mat;
}

float MotionCVResize::get_scale_ratio(void) const {
    return scale_ratio;
}

#endif
