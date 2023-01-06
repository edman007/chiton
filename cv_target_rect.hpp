#ifndef __CV_TARGET_RECT_HPP__
#define __CV_TARGET_RECT_HPP__
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
#include <opencv2/core.hpp>
#include "chiton_ffmpeg.hpp"

//helper class to track trargets
class TargetRect {
public:
    TargetRect(const cv::RotatedRect &rect);//create a new target from a given rotated rect
    TargetRect(const TargetRect &old_target, const cv::RotatedRect &new_rect, const AVFrame *cur_frame);//create a new target that updates the rect of an old target
    TargetRect(const TargetRect &rhs);//copy constructor
    TargetRect(TargetRect &&rhs) noexcept;//move constructor
    ~TargetRect();//destructor
    const cv::RotatedRect& get_rect(void) const;//return a reference to the underlying rect
    bool is_valid(void)const ;//return true is it has not been marked invalid
    void mark_invalid(void);//mark this rect as invalid
    int get_count(void) const;//get the number of times this has been seen
    const AVFrame *get_best_frame(void) const;//return the best frame found
    const cv::RotatedRect& get_best_rect(void) const;//return the rect for the best frame
    void scale(float scale);//Applies the scaling ratio
private:
    cv::RotatedRect rect, best_rect;//the underlying rects
    int count;//how many times we have seen this
    bool valid;//is this is still valid
    AVFrame *frame;//the best frame found so far
    void scale_rect(float scale, cv::RotatedRect &rect);//scale the target Rect
};

#endif
#endif
