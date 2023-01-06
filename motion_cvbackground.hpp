#ifndef __MOTION_ALGO_CVBACKGROUND_HPP__
#define __MOTION_ALGO_CVBACKGROUND_HPP__
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
#include "motion_cvresize.hpp"
#include "filter.hpp"
#include <opencv2/core.hpp>

class MotionCVBackground : public MotionAlgo {
public:
    MotionCVBackground(Config &cfg, Database &db, MotionController &controller);
    ~MotionCVBackground();
    bool process_frame(const AVFrame *frame, bool video);//process the frame, return false on error
    bool set_video_stream(const AVStream *stream, const AVCodecContext *codec);//identify the video stream
    static const std::string& get_mod_name(void);//return the name of the algorithm
    const cv::UMat get_background(void);//get the averaged background
    bool init(void);//called immeditly after the constructor to allow dependicies to be setup
private:
    MotionCVResize *ocvr;
    cv::UMat avg;//the 16-bit average image
    cv::UMat low_res;//low res version derived from the 16-bit
    float tau;
};

#endif
#endif
