#ifndef __MOTION_ALGO_OPENCV_HPP__
#define __MOTION_ALGO_OPENCV_HPP__
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
#include "filter.hpp"
#include <opencv2/core.hpp>

class MotionOpenCV : public MotionAlgo {
public:
    MotionOpenCV(Config &cfg, Database &db, MotionController &controller);
    ~MotionOpenCV();
    bool process_frame(const AVFrame *frame, bool video);//process the frame, return false on error
    bool set_video_stream(const AVStream *stream, const AVCodecContext *codec);//identify the video stream
    const std::string& get_name(void);//return the name of the algorithm
    const cv::UMat& get_UMat(void);//return the UMat (CV_8UC1) for this frame
private:
    cv::UMat buf_mat;//the Mat we operate on
    cv::Mat input_mat;//the sw mat
    cv::UMat tmp1, tmp2;//temporary buffers
    AVFrame *input;
    Filter fmt_filter;
    bool map_cl;//true if using opencl mapping instead of direct vaapi mapping
    bool map_indirect;//true if mapping indirectly from VA-API to SW OpenCV
#ifdef HAVE_OPENCL
    void map_ocl_frame(AVFrame *input);
#endif
#ifdef HAVE_VAAPI
    void indirect_vaapi_map(const AVFrame *input);
#endif
};

class MotionOpenCVAllocator : public MotionAlgoAllocator {
    MotionAlgo* allocate(Config &cfg, Database &db, MotionController &controller) {return new MotionOpenCV(cfg, db, controller);};
    const std::string& get_name(void);
};

#endif
#endif
