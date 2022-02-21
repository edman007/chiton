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

MotionController::MotionController(Database &db, Config &cfg) : db(db), cfg(cfg) {
    video_idx = -1;
    audio_idx = -1;
    //register all known algorithms
#ifdef HAVE_OPENCV
    register_motion_algo(new MotionOpenCVAllocator());
    register_motion_algo(new MotionCVBackgroundAllocator());
    register_motion_algo(new MotionCVMaskAllocator());
    register_motion_algo(new MotionCVDetectAllocator());
#ifdef DEBUG
    register_motion_algo(new MotionCVDebugShowAllocator());
#endif
#endif
    add_algos();
}

MotionController::~MotionController(){
    clear_algos();
    for (auto &ma : supported_algos){
        delete ma;
    }
}

void MotionController::clear_algos(void){
    for (auto &ma : algos){
        delete ma;
    }
    algos.clear();
}
bool MotionController::process_frame(int index, const AVFrame *frame){
    bool video = index == video_idx;
    if (!video && index != audio_idx){
        return false;//unknown stream
    }
    bool ret = true;
    for (auto &ma : algos){
        ret &= ma->process_frame(frame, video);
    }
    return ret;
}

bool MotionController::set_video_stream(const AVStream *stream, const AVCodecContext *codec){
    if (!stream){
        return false;//there is no video stream
    }
    video_idx = stream->id;
    bool ret = true;
    for (auto &ma : algos){
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
    for (auto &ma : algos){
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

void MotionController::register_motion_algo(MotionAlgoAllocator* maa){
    if (maa){
        supported_algos.push_back(maa);
    }
}

bool MotionController::parse_algos(const std::string param, std::vector<std::string> &algo_list){
    const std::string &str_list = cfg.get_value(param);
    std::string::size_type start = 0;
    while (start < str_list.length()){
        auto end = str_list.find(":", start);
        if (end == start){
            start++;
            continue;
        }
        algo_list.push_back(str_list.substr(start, end));
        if (end == std::string::npos){
            break;
        }
        start = end + 1;
    }
    return !algo_list.empty();
}

MotionAlgo* MotionController::find_algo(const std::string &name){
    for (auto &ma : algos){
        if (ma->get_name() == name){
            return ma;
        }
    }
    for (auto &ma : supported_algos){
        if (ma->get_name() == name){
            algos.push_back(ma->allocate(cfg, db, *this));
            algos.back()->init();
            return algos.back();
        }
    }
    return NULL;
}

MotionAlgo* MotionController::get_algo_before(const std::string &name, const MotionAlgo *algo){
    for (auto it = algos.begin(); it != algos.end(); it++){
        if (*it == algo){
            for (auto &ma : supported_algos){
                if (ma->get_name() == name){
                    MotionAlgo *new_algo = ma->allocate(cfg, db, *this);
                    algos.insert(it, new_algo);
                    new_algo->init();
                    return new_algo;
                }
            }
            return NULL;//not found
        }
        if ((*it)->get_name() == name){
            return *it;
        }
    }
    return NULL;//algo was not found
}

bool MotionController::add_algos(void){
    std::vector<std::string> algo_list;
    if (!parse_algos("motion-algos", algo_list)){
        return true;//nothing to configure
    }
    if (algo_list[0] == "" || algo_list[0] == "none"){
        return true;//explicitly disabled
    }

    bool ret = false;
    for (auto &name : algo_list){
        auto ma = find_algo(name);
        if (ma != NULL){
            ret |= true;
        } else {
            LWARN("Cannot find motion algorithm '" + name + "', skipping");
        }
    }

    return true;
}
