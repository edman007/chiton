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
 *   Copyright 2022-2023 Ed Martin <edman007@edman007.com>
 *
 **************************************************************************
 */

#include "cv_target_rect.hpp"
#ifdef HAVE_OPENCV
#include "util.hpp"

TargetRect::TargetRect(const cv::RotatedRect &rect) : rect(rect) {
    count = 0;
    valid = true;
    frame = NULL;//we never actually allocate for the first frame
}

TargetRect::TargetRect(const TargetRect &old_target, const cv::RotatedRect &new_rect, const AVFrame *cur_frame){
    frame = av_frame_alloc();
    if (old_target.frame && old_target.rect.size.area() > new_rect.size.area()){
        int ret = av_frame_ref(frame, old_target.frame);
        if (ret){
            LERROR("av_frame_ref failed TargetRect::1");
        }
        best_rect = old_target.rect;
    } else {
        int ret = av_frame_ref(frame, cur_frame);
        if (ret){
            LERROR("av_frame_ref failed Targetrect::2");
        }
        best_rect = new_rect;
    }
    rect = new_rect;
    valid = true;
    count = old_target.count + 1;
}

TargetRect::TargetRect(const TargetRect &rhs){
    rect = rhs.rect;
    best_rect = rhs.best_rect;
    count = rhs.count;
    valid = rhs.valid;
    if (rhs.frame){
        //alloc and copy the frame
        frame = av_frame_alloc();
        int ret = av_frame_ref(frame, rhs.frame);
        if (ret){
            LERROR("av_frame_ref failed TargetRect::3");
        }
    } else {
        frame = NULL;
    }
    LDEBUG("Slow copy of TargetRect");
}

TargetRect::TargetRect(TargetRect &&rhs) noexcept
    : rect(std::move(rhs.rect)), best_rect(std::move(rhs.best_rect)) {
    count = rhs.count;
    valid = rhs.valid;
    frame = rhs.frame;
    rhs.frame = NULL;
}

TargetRect::~TargetRect(){
    av_frame_free(&frame);
}

const cv::RotatedRect& TargetRect::get_rect(void) const {
    return rect;
}

bool TargetRect::is_valid(void) const{
    return valid;
}

void TargetRect::mark_invalid(void){
    valid = false;
}

int TargetRect::get_count(void) const{
    return count;
}

void TargetRect::scale(float s) {
    if (s == 1){
        return;
    }
    scale_rect(s, rect);
    scale_rect(s, best_rect);
}

void TargetRect::scale_rect(float s, cv::RotatedRect &rect) {
    rect.center.x /= s;
    rect.center.y /= s;
    rect.size.width /= s;
    rect.size.height /= s;
}

const AVFrame *TargetRect::get_best_frame(void) const {
    return frame;
}

const cv::RotatedRect& TargetRect::get_best_rect(void) const {
    return best_rect;
}

//HAVE_OPENCV
#endif
