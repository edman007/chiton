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
#include "motion_controller.hpp"
#include "util.hpp"
#include "motion_opencv.hpp"
#include "motion_cvbackground.hpp"
#include "motion_cvmask.hpp"
#include "motion_cvdetect.hpp"
#include "motion_cvdebugshow.hpp"

MotionController::MotionController(Database &db, Config &cfg, StreamUnwrap &stream) : ModuleController<MotionAlgo, MotionController>(cfg, db, "motion"), stream(stream), events(cfg, db)  {
    video_idx = -1;
    audio_idx = -1;
    //register all known algorithms
#ifdef HAVE_OPENCV
    register_module(new ModuleFactory<MotionOpenCV, MotionAlgo, MotionController>());
    register_module(new ModuleFactory<MotionCVBackground, MotionAlgo, MotionController>());
    register_module(new ModuleFactory<MotionCVMask, MotionAlgo, MotionController>());
    register_module(new ModuleFactory<MotionCVDetect, MotionAlgo, MotionController>());
#ifdef DEBUG
    register_module(new ModuleFactory<MotionCVDebugShow, MotionAlgo, MotionController>());
#endif
#endif
    add_mods();
}

MotionController::~MotionController(){

}

bool MotionController::process_frame(int index, const AVFrame *frame){
    bool video = index == video_idx;
    if (!video && index != audio_idx){
        return false;//unknown stream
    }
    bool ret = true;
    for (auto &ma : mods){
        ret &= ma->process_frame(frame, video);
    }
    return ret;
}

bool MotionController::set_streams(void){
    bool ret = set_video_stream(stream.get_video_stream(), stream.get_codec_context(stream.get_video_stream()));
    ret &= set_audio_stream(stream.get_audio_stream(), stream.get_codec_context(stream.get_audio_stream()));
    return ret;
}

bool MotionController::set_video_stream(const AVStream *stream, const AVCodecContext *codec){
    if (!stream){
        return false;//there is no video stream
    }
    video_idx = stream->id;
    bool ret = true;
    for (auto &ma : mods){
        ret &= ma->set_video_stream(stream, codec);
    }

    return ret;
}

bool MotionController::set_audio_stream(const AVStream *stream, const AVCodecContext *codec){
    if (!stream){
        return false;//there is no audio stream
    }
    audio_idx = stream->id;
    bool ret = true;
    for (auto &ma : mods){
        ret &= ma->set_audio_stream(stream, codec);
    }
    return ret;
}

bool MotionController::decode_video(void){
    return true;
}

bool MotionController::decode_audio(void){
    return false;//unsupported
}

void MotionController::get_frame_timestamp(const AVFrame *frame, bool video, struct timeval &time){
    if (video){
        stream.timestamp(frame, video_idx, time);
    } else {
        stream.timestamp(frame, audio_idx, time);
    }
}

EventController& MotionController::get_event_controller(void){
    return events;
}
