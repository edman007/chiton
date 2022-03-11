#ifndef __MOTION_ALGO_CVMASK_HPP__
#define __MOTION_ALGO_CVMASK_HPP__
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

#include "config_build.hpp"
#ifdef HAVE_OPENCV
#include "motion_algo.hpp"
#include "motion_opencv.hpp"
#include "motion_cvbackground.hpp"
#include <opencv2/core.hpp>

class MotionCVMask : public MotionAlgo {
public:
    MotionCVMask(Config &cfg, Database &db, MotionController &controller);
    ~MotionCVMask();
    bool process_frame(const AVFrame *frame, bool video);//process the frame, return false on error
    bool set_video_stream(const AVStream *stream, const AVCodecContext *codec);//identify the video stream
    static const std::string& get_mod_name(void);//return the name of the algorithm
    bool init(void);//called immeditly after the constructor to allow dependicies to be setup
    const cv::UMat get_masked(void);//returns a CV_8UC1 (static bits masked out)
private:
    MotionOpenCV *ocv;
    MotionCVBackground *background;
    cv::UMat masked;//the masked image
    cv::UMat sensitivity;
    float tau;//sensitivity tau (time constant)
    float beta;//sensitivity beta (diff amplification factor)
    float thresh;//final thresholding value
};

#endif
#endif
