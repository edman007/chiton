#ifndef __MOTION_CONTROLLER_HPP__
#define __MOTION_CONTROLLER_HPP__
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

#include "config_build.hpp"
#include "chiton_ffmpeg.hpp"
#include "chiton_config.hpp"
#include "module_controller.hpp"
#include "image_util.hpp"
class MotionController;
#include "event_controller.hpp"
#include "motion_algo.hpp"
#include <list>
#include "stream_unwrap.hpp"
#include "event_notification.hpp"


//this class runs all motion detection algorithms
class MotionController : public ModuleController<MotionAlgo, MotionController> {
public:
    MotionController(Database &db, Config &cfg, StreamUnwrap &stream, ImageUtil &img);
    ~MotionController();
    bool process_frame(int index, const AVFrame *frame, bool &skipped);//process the frame, return false on error, skipped is set to true if frame was skipped
    bool set_streams(void);//identify the streams (by looking at the StreamUnwrap instance)
    bool decode_video(void);//true if video is required (configured to do video motion detection)
    bool decode_audio(void);//true if audio is required (configured to do audio motion  detection)
    //returns the motion algorithm with name, will be executed before algo, returns null if the algorithm does not exist, calls init() on the algorithm
    void get_frame_timestamp(const AVFrame *frame, bool video, struct timeval &time);//wrapper for StreamUnwrap timestamp(), assumes from video stream if video is true
    EventController& get_event_controller(void);//return a handle to the event controller
    ImageUtil& get_img_util(void);//return the imageutil object
private:
    StreamUnwrap &stream;
    EventController events;
    ImageUtil &img;

    int video_idx;//index of the video stream
    int audio_idx;//index of the audio stream

    double skip_ratio;//the configured skip_ratio
    bool set_video_stream(const AVStream *stream, const AVCodecContext *codec);//identify the video stream
    bool set_audio_stream(const AVStream *stream, const AVCodecContext *codec);//identify the audio stream

    bool should_skip(void);//check if we are falling behind and skip if required, return true when we need to skip
};

#endif
