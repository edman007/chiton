#ifndef __MOTION_ALGO_CVDETECT_HPP__
#define __MOTION_ALGO_CVDETECT_HPP__
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
#include "motion_cvmask.hpp"
#include <opencv2/core.hpp>
#include "motion_opencv.hpp"
#include "motion_cvresize.hpp"
#include "cv_target_rect.hpp"

//algorithm to implement the cvdetect motion detection algorithm
class MotionCVDetect : public MotionAlgo {
public:
    MotionCVDetect(Config &cfg, Database &db, MotionController &controller);
    ~MotionCVDetect();
    bool process_frame(const AVFrame *frame, bool video);//process the frame, return false on error
    bool set_video_stream(const AVStream *stream, const AVCodecContext *codec);//identify the video stream
    static const std::string& get_mod_name(void);//return the name of the algorithm
    bool init(void);//called immeditly after the constructor to allow dependicies to be setup
    const cv::UMat& get_debug_view(void);
private:
    MotionCVMask *masked_objects;//mask we are using
    std::vector<std::vector<cv::Point>> contours;//the detected contours
    std::vector<TargetRect> targets;//the targets we are counting
    cv::UMat debug_view, canny;
    MotionOpenCV *ocv;
    MotionCVResize *ocvr;
    void display_objects(void);//debug tool to display the objects
    void send_events(void);

    //config parameters
    float min_dist;//min distance between objects to track seperatly
    float min_area;//miniumn area of an object to track
    int tracking_time_send;//number of frames tracked to trigger an event
};

#endif
#endif
