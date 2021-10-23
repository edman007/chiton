#ifndef __HWLOAD_HPP__
#define __HWLOAD_HPP__
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
 *   Copyright 2021 Ed Martin <edman007@edman007.com>
 *
 **************************************************************************
 */

#include "chiton_config.hpp"
#include "chiton_ffmpeg.hpp"
extern "C" {
#include <libavfilter/avfilter.h>
};
class Filter {
    /*
     * Simple Wrapper to implement hwload/unload filter as required
     *
     */
public:
    Filter(Config& cfg);
    ~Filter();

    //set the target pixel format
    //returns true if format was accepted
    bool set_target_fmt(const AVPixelFormat fmt, AVCodecID codec_id, int codec_profile);

    void set_source_time_base(const AVRational time_base);
    bool send_frame(const AVFrame *frame);//refs the frame into this loader
    bool get_frame(AVFrame *frame);//refs a frame into the supplied arg, caller must av_frame_unref() it
    bool peek_frame(AVFrame *frame);//refs a frame into the supplied arg, caller must av_frame_unref() it, next send_frame() will be a NO-OP, and get frame will still return this frame
private:
    bool is_hw(const AVPixelFormat fmt) const;//return true if format is supproted HW format
    bool is_hw(const int fmt) const;//is_hw() with a cast to AVPixelFormat
    std::string get_filter_str(const AVFrame *frame) const;//return the string for the filter
    bool init_filter(const AVFrame *frame);

    Config &cfg;
    bool init_complete;//true if init completed successfully
    bool passthrough;//true if we are operating in passthrough mode
    bool peeked;//true if peek frame was called previously
    AVRational time_base;
    AVPixelFormat target_fmt;//fmt that we are converting t
    AVCodecID target_codec;
    int target_profile;
    AVFrame *tmp_frame;//used to passthrough a frame when no conversion is required and to implement peeking
    bool tmp_frame_filled;
    AVFilterGraph *filter_graph;
    AVFilterContext *buffersink_ctx;
    AVFilterContext *buffersrc_ctx;


};

#endif
