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

#include "motion_cvdetect.hpp"
#ifdef HAVE_OPENCV
#include "util.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/video/tracking.hpp>

static const std::string algo_name = "cvdetect";

MotionCVDetect::MotionCVDetect(Config &cfg, Database &db, MotionController &controller) : MotionAlgo(cfg, db, controller) {
    masked_objects = NULL;
    ocv = NULL;
}

MotionCVDetect::~MotionCVDetect(){
    masked_objects = NULL;
}

bool MotionCVDetect::process_frame(const AVFrame *frame, bool video){
    if (!video || !masked_objects){
        return true;
    }

    //find contours
    float thresh = 150.0;
    //cv::Canny(masked_objects->get_masked(), canny, thresh, thresh*2 );
    //cv::findContours(canny, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);
    cv::findContours(masked_objects->get_masked(), contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

    std::vector<TargetRect> new_targets;
    float min_area = 150.0;
    //compute the bounding rect for all
    float min_dist = thresh*thresh;
    for (auto ctr : contours){
        auto new_rect = cv::minAreaRect(ctr);
        if (new_rect.size.area() < min_area){
            continue;//toss the small spots
        }
        float nearest_dist = std::numeric_limits<float>::max();
        bool found_new = false;
        for (auto &old_target : targets){
            if (!old_target.is_valid()){
                continue;
            }
            float x = old_target.get_rect().center.x - new_rect.center.x;
            float y = old_target.get_rect().center.y - new_rect.center.y;
            x *= x;
            y *= y;
            float dist = x + y;
            if (dist < min_dist){
                //too close, just replace this target with the new contour
                new_targets.push_back(TargetRect(old_target, new_rect, frame));
                old_target.mark_invalid();
                found_new = true;
                LDEBUG("Tracked Target!");
                break;
            }
        }
        if (!found_new){
            new_targets.push_back(TargetRect(new_rect));
        }
    }
    targets = std::move(new_targets);
    if (targets.size() > 0){
        LDEBUG("Tracking " + std::to_string(targets.size()) + " targets");
    }

    send_events();
    display_objects();
    return true;
}

bool MotionCVDetect::set_video_stream(const AVStream *stream, const AVCodecContext *codec) {
    return true;
}

const std::string& MotionCVDetect::get_name(void) {
    return algo_name;
}

const std::string& MotionCVDetectAllocator::get_name(void) {
    return algo_name;
}

bool MotionCVDetect::init(void) {
    ocv = static_cast<MotionOpenCV*>(controller.get_algo_before("opencv", this));
    masked_objects = static_cast<MotionCVMask*>(controller.get_algo_before("cvmask", this));
    return true;
}

void MotionCVDetect::display_objects(void){
    //ocv->get_UMat().copyTo(debug_view);
    masked_objects->get_masked().copyTo(debug_view);
    cv::RNG rng(12345);
    for (auto &target : targets){
        if (target.get_count() < 10){
            continue;
        }
        LDEBUG("Drawing Tracked Target!");
        cv::Scalar color = cv::Scalar( rng.uniform(0, 256), rng.uniform(0,256), rng.uniform(0,256) );
        cv::Point2f vertices[4];
        target.get_rect().points(vertices);
        for ( int j = 0; j < 4; j++ ){
            line(debug_view, vertices[j], vertices[(j+1)%4], color);
        }
        LDEBUG("Size: " + std::to_string(target.get_rect().size.area()));
    }
}

const cv::UMat &MotionCVDetect::get_debug_view(void){
    return debug_view;
}

void MotionCVDetect::send_events(void){
    for (auto &target : targets){
        if (target.get_count() == 10){
            Event &e = controller.get_event_controller().get_new_event();
            e.set_position(target);
            struct timeval time;
            controller.get_frame_timestamp(target.get_best_frame(), true, time);
            e.set_frame(target.get_best_frame());
            e.set_timestamp(time);
            e.set_source(algo_name);
            LWARN("Sending Event");
            controller.get_event_controller().send_event(e);
        }
    }
}

//HAVE_OPENCV
#endif