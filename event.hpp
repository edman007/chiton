#ifndef __EVENT_HPP__
#define __EVENT_HPP__
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
#include "chiton_ffmpeg.hpp"
#include "chiton_config.hpp"
#ifdef HAVE_OPENCV
#include "cv_target_rect.hpp"
#endif


//this class is a single event instance
class Event {
public:
    Event(Config &cfg);
    ~Event();

    void clear();//initilizes the event for future use
    void invalidate(void);//marks the event as invalid
    bool is_valid(void);//return if this Event is currently in use

    //must set timestamp and source
    //frame and position should be set for video
    bool set_timestamp(struct timeval &etime);//set the timestamp of the event (currently, the timestamp of the thumbnail)
    const struct timeval &get_timestamp(void);//get the timestamp of the event

    bool set_position(float x0, float y0, float x1, float y1);//set the position of the event
    const std::vector<float>& get_position();//returns a vector of 4 float, in this order: x0, y0, x1, y1
#ifdef HAVE_OPENCV
    bool set_position(const TargetRect &rect);//set the position of the event
#endif

    bool set_frame(const AVFrame *frame);//set the frame
    const AVFrame* get_frame(void);//return the frame that the event was found in

    bool set_source(const std::string &name);//set the name of the source module
    const std::string& get_source(void);//get the name of the source module

    bool set_score(float score);//set the score of the event (0-100)
    float get_score(void);//get the score of the event (0-100)

private:
    Config &cfg;
    AVFrame *src;
    struct timeval time;
    std::vector<float> pos;
    std::string source;
    bool valid;
    float score;
};
#endif
