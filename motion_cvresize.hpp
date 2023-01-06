#ifndef __MOTION_ALGO_CVRESIZE_HPP__
#define __MOTION_ALGO_CVRESIZE_HPP__
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

#include "config_build.hpp"
#ifdef HAVE_OPENCV
#include "motion_algo.hpp"
#include "filter.hpp"
#include "motion_opencv.hpp"
#include <opencv2/core.hpp>

class MotionCVResize : public MotionAlgo {
public:
    MotionCVResize(Config &cfg, Database &db, MotionController &controller);
    ~MotionCVResize();
    bool process_frame(const AVFrame *frame, bool video);//process the frame, return false on error
    bool set_video_stream(const AVStream *stream, const AVCodecContext *codec);//identify the video stream
    bool init(void);//called immeditly after the constructor to allow dependicies to be setup
    static const std::string& get_mod_name(void);//return the name of the algorithm
    const cv::UMat& get_UMat(void) const;//return the UMat (CV_8UC1) for this frame
    float get_scale_ratio(void) const;//return the scale ratio (0-1)
private:
    cv::UMat buf_mat;//the output UMat
    float scale_ratio;//the configured scale ratio
    MotionOpenCV *ocv;//the ocv source object
};


#endif
#endif
