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

#include "event.hpp"
#include "util.hpp"

Event::Event(Config &cfg) : cfg(cfg) {
    src = NULL;
    clear();
};

Event::~Event(){
    av_frame_free(&src);
}

bool Event::set_timestamp(struct timeval &etime){
    time.tv_sec = etime.tv_sec;
    time.tv_usec = etime.tv_usec;
    return true;
}

const struct timeval &Event::get_timestamp(void){
    return time;
}
bool Event::set_position(float x0, float y0, float x1, float y1){
    pos.reserve(4);
    pos[0] = x0;
    pos[1] = y0;
    pos[2] = x1;
    pos[3] = y1;
    return true;
}

#ifdef HAVE_OPENCV
bool Event::set_position(const TargetRect &rect){
    const cv::RotatedRect &r = rect.get_best_rect();
    const cv::Rect b_rect = r.boundingRect();//upright rect
    auto tl = b_rect.tl();//top left
    auto br = b_rect.br();//bottom right
    pos.reserve(4);
    pos[0] = tl.x;
    pos[1] = tl.y;
    pos[2] = br.x;
    pos[3] = br.y;
    return true;
}
#endif

const std::vector<float>& Event::get_position(void){
    return pos;
}

bool Event::set_frame(const AVFrame *frame){
    if (!src){
        src = av_frame_alloc();
    } else {
        av_frame_unref(src);
    }
    av_frame_ref(src, frame);
    return true;
}

const AVFrame* Event::get_frame(void){
    return src;
}

void Event::clear(){
    if (src){
        //FIXME: Should this actually deallocate it, or should we have logic to just act as if it was deallocated to avoid excessive allocation
        av_frame_unref(src);
        av_frame_free(&src);
    }
    src = NULL;
    time.tv_sec = 0;
    time.tv_usec = 0;
    source = "?";
    valid = true;
}

void Event::invalidate(void){
    valid = false;
}

bool Event::is_valid(void){
    return valid;
}

bool Event::set_source(const std::string &name){
    source = name;
    return true;
}
const std::string& Event::get_source(void){
    return source;
}
