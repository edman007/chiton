#ifndef __MOTION_ALGO_HPP__
#define __MOTION_ALGO_HPP__
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

class MotionAlgo;
#include "motion_controller.hpp"

//this class runs all motion detection algorithms
class MotionAlgo : public Module<MotionAlgo, MotionController> {
public:
    MotionAlgo(Config &cfg, Database &db, MotionController &controller, const std::string &name) : Module<MotionAlgo, MotionController>(cfg, db, controller, name) {};
    virtual ~MotionAlgo() {};
    virtual bool process_frame(const AVFrame *frame, bool video) = 0;//process the frame, return false on error, video is true if video frame is supplied
    virtual bool set_video_stream(const AVStream *stream, const AVCodecContext *codec) {return true;};//identify the video stream
    virtual bool set_audio_stream(const AVStream *stream, const AVCodecContext *codec) {return true;};//identify the audio stream
};
#endif
