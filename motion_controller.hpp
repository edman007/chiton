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
 *   Copyright 2022 Ed Martin <edman007@edman007.com>
 *
 **************************************************************************
 */

#include "config_build.hpp"
#include "chiton_ffmpeg.hpp"
#include "chiton_config.hpp"
class MotionController;
class MotionAlgoAllocator;
#include "motion_algo.hpp"
#include <vector>
#include <list>
#include "stream_unwrap.hpp"
#include "event_controller.hpp"

//this class runs all motion detection algorithms
class MotionController {
public:
    MotionController(Database &db, Config &cfg, StreamUnwrap &stream);
    ~MotionController();
    bool process_frame(int index, const AVFrame *frame);//process the frame, return false on error
    bool set_streams(void);//identify the streams (by looking at the StreamUnwrap instance)
    bool decode_video(void);//true if video is required (configured to do video motion detection)
    bool decode_audio(void);//true if audio is required (configured to do audio motion  detection)
    //returns the motion algorithm with name, will be executed before algo, returns null if the algorithm does not exist, calls init() on the algorithm
    MotionAlgo* get_algo_before(const std::string &name, const MotionAlgo *algo);
    void register_motion_algo(MotionAlgoAllocator* maa);//registers an allocator and makes an algorithm available for use
    void get_frame_timestamp(const AVFrame *frame, bool video, struct timeval &time);//wrapper for StreamUnwrap timestamp(), assumes from video stream if video is true
    EventController& get_event_controller(void);//return a handle to the event controller
private:
    Database &db;
    Config &cfg;
    StreamUnwrap &stream;
    EventController events;

    std::vector<MotionAlgoAllocator*> supported_algos;

    int video_idx;
    int audio_idx;
    std::list<MotionAlgo*> algos;

    bool set_video_stream(const AVStream *stream, const AVCodecContext *codec);//identify the video stream
    bool set_audio_stream(const AVStream *stream, const AVCodecContext *codec);//identify the audio stream
    void clear_algos(void);//clear & delete algos
    bool parse_algos(const std::string param, std::vector<std::string> &algo_list);//read the config value param, write out all algorithms to algo_list, return true if non-empty
    MotionAlgo* find_algo(const std::string &name);//return the algo with a given name, null if it does not exist, allocate it if not already done
    bool add_algos(void);//put all algos in algos
    bool init(void);//init all algos
};

#endif
