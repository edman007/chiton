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
 *   Copyright 2021-2022 Ed Martin <edman007@edman007.com>
 *
 **************************************************************************
 */
#include "filter.hpp"
#include "util.hpp"
#include <sstream>
extern "C" {
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
};
//supproted HW methods, these trigger hwdownload/hwupload
const AVPixelFormat hw_formats[] = {
    AV_PIX_FMT_VAAPI,
    AV_PIX_FMT_VDPAU,
    AV_PIX_FMT_OPENCL
};

Filter::Filter(Config& cfg) : cfg(cfg) {
    target_fmt = AV_PIX_FMT_NONE;
    tmp_frame = av_frame_alloc();
    init_complete = false;
    passthrough = false;
    time_base = {1, 1};
    filter_graph = NULL;
    buffersink_ctx = NULL;
    buffersrc_ctx = NULL;
    peeked = false;
    tmp_frame_filled = false;
    target_codec = AV_CODEC_ID_NONE;
    target_profile = 0;
}

Filter::~Filter(){
    av_frame_free(&tmp_frame);
    avfilter_graph_free(&filter_graph);
}

bool Filter::set_target_fmt(const AVPixelFormat fmt, AVCodecID codec_id, int codec_profile){
    target_fmt = fmt;
    target_codec = codec_id;
    target_profile = codec_profile;
    return fmt != AV_PIX_FMT_NONE && target_codec != AV_CODEC_ID_NONE;
}

bool Filter::send_frame(const AVFrame *frame){
    if (!init_complete){
        if (!init_filter(frame)){
            return false;
        }
    }
    if (peeked){
        LDEBUG("Peeked received again, dropping");
        return true;//NO-OP, we actually already got it
    }
    if (passthrough){
        if (tmp_frame_filled){
            return false;
        } else {
            av_frame_ref(tmp_frame, frame);
            tmp_frame_filled = true;
            return true;
        }
    }

    if (av_buffersrc_write_frame(buffersrc_ctx, frame) < 0) {
        LERROR("Error while feeding the filtergraph");
        return false;
    }

    return true;
}

bool Filter::get_frame(AVFrame *frame){
    if (passthrough){
        if (tmp_frame_filled){
            av_frame_unref(frame);
            av_frame_move_ref(frame, tmp_frame);
            tmp_frame_filled = false;
            peeked = false;
            return true;
        } else {
            return false;
        }
    }
    //if peek was called before
    if (tmp_frame_filled){
        av_frame_move_ref(frame, tmp_frame);
        tmp_frame_filled = false;
        peeked = false;
        return true;
    }
    int ret = av_buffersink_get_frame(buffersink_ctx, frame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
        //the end, not an error
        return false;
    }

    if (ret < 0){
        LWARN("Error getting from from filter");
        return false;
    }

    return true;
}

bool Filter::peek_frame(AVFrame *frame){
    if (passthrough){
        if (tmp_frame_filled){
            av_frame_ref(frame, tmp_frame);
            tmp_frame_filled = false;
            return true;
        } else {
            return false;
        }
    }
    if (tmp_frame_filled){
        return false;
    }

    int ret = av_buffersink_get_frame(buffersink_ctx, tmp_frame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
        //the end, not an error
        return false;
    }

    if (ret < 0){
        LWARN("Error getting from from filter");
        return false;
    }
    av_frame_ref(frame, tmp_frame);
    tmp_frame_filled = true;
    peeked = true;
    return true;
}



bool Filter::init_filter(const AVFrame *frame){
    if (frame->format == target_fmt){
        //do NO-OP
        //FIXME: Check for user options
        passthrough = true;
        return true;
    }

    std::stringstream args;
    const AVFilter *buffersrc  = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    enum AVPixelFormat pix_fmts[] = { target_fmt, AV_PIX_FMT_NONE };

    filter_graph = avfilter_graph_alloc();
    if (!filter_graph) {
        LWARN("Failed to alloc filter");
        return false;
    }


    args << "video_size=" << frame->width << "x" << frame->height;
    args << ":pix_fmt=" << frame->format;
    args << ":time_base=" << time_base.num << "/" << time_base.den;
    args << ":pixel_aspect=" << frame->sample_aspect_ratio.num << "/" << frame->sample_aspect_ratio.den;
    LDEBUG("Source Filter Format: " + args.str());
    int ret = 0;
    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                                       args.str().c_str(), NULL, filter_graph);
    if (ret < 0) {
        LERROR("Cannot create filter buffer source");
        return false;
    }

    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", NULL, NULL, filter_graph);
    if (ret < 0) {
        LERROR("Cannot create filter buffer sink");
        return false;
    }


    if (frame->hw_frames_ctx){//copy the input device context if one exists
        AVBufferSrcParameters *bsp = av_buffersrc_parameters_alloc();
        bsp->hw_frames_ctx = av_buffer_ref(frame->hw_frames_ctx);
        bsp->format = frame->format;
        bsp->width = frame->width;
        bsp->height = frame->height;
        bsp->sample_aspect_ratio = frame->sample_aspect_ratio;
        if (av_buffersrc_parameters_set(buffersrc_ctx, bsp)){
            LWARN("Failed to set filter buffersrc parameters");
        }
        av_free(bsp);
    } else if (!is_hw(target_fmt)){
        LDEBUG("Performing SW/SW filter");
    }//SW->HW is added after


    //specify output format
    ret = av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        LERROR("Cannot set filter output pixel format");
        return false;
    }

    //init the end points
    AVFilterInOut *outputs = avfilter_inout_alloc();
    if (!outputs){
        LERROR("Failed to alloc filter output");
        return false;
    }

    AVFilterInOut *inputs  = avfilter_inout_alloc();
    if (!inputs){
        LERROR("Failed to alloc filter input");
        avfilter_inout_free(&outputs);
        return false;
    }
    //connect in/out context
    outputs->name       = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    inputs->name       = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;

    //parse the filter text
    ret = avfilter_graph_parse_ptr(filter_graph, get_filter_str(frame).c_str(), &inputs, &outputs, NULL);
    if (ret >= 0){
        //hwmap specifically needs
        for (unsigned int i = 0; i < filter_graph->nb_filters; i++){
            AVFilterContext *flt = filter_graph->filters[i];
            if (strcmp(flt->filter->name, "hwmap") == 0 || strcmp(flt->filter->name, "hwupload") == 0){
                if (!frame->hw_frames_ctx && target_fmt == AV_PIX_FMT_VAAPI) {//source is SW dest is VAAPI
                    flt->hw_device_ctx = gcff_util.get_vaapi_ctx(target_codec, target_profile, frame->width, frame->height);
                }
                if (target_fmt == AV_PIX_FMT_OPENCL) {//dest is OpenCL
                    flt->hw_device_ctx = gcff_util.get_opencl_ctx(target_codec, target_profile, frame->width, frame->height);
                }

            }
        }
        ret = avfilter_graph_config(filter_graph, NULL);
        if (ret < 0){
            LERROR("Failed to validate filter");
        }
    } else {
        LERROR("Failed to parse filter graph");
    }

    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    if (ret >= 0){
        init_complete = true;;
    }
    return ret >= 0;
}

bool Filter::is_hw(const AVPixelFormat fmt) const{
    for (auto &hw : hw_formats){
        if (hw == fmt){
            return true;
        }
    }
    return false;
}

bool Filter::is_hw(const int fmt) const {
    return is_hw(static_cast<AVPixelFormat>(fmt));
}
std::string Filter::get_filter_str(const AVFrame *frame) const {
    std::stringstream filters_txt;
    //test if we will need to start with a format)
    if (is_hw(frame->format) || is_hw(target_fmt)){
        //have a SW format and the SW decoder didn't get it into something compatable, so format into something compatable
        if (!is_hw(frame->format) &&
            (!gcff_util.sw_format_is_hw_compatable(static_cast<enum AVPixelFormat>(frame->format)) || cfg.get_value("video-hw-pix-fmt") != "auto")){
            filters_txt << "format=pix_fmts=" << gcff_util.get_sw_hw_format_list(cfg) << ",";
        }

        //transfer between HW...HWMap?
        //hwmap is supposed to work in all cases here...but in testing it doesn't actually work, not sure why
        if (!is_hw(frame->format)){
            filters_txt << "hwupload=";
        } else if (!is_hw(target_fmt)){
            filters_txt << "hwdownload,format=pix_fmts=" << gcff_util.get_sw_hw_format_list(cfg);//request to download into supported format, then format later
        } else {
            filters_txt << "hwmap=mode=read";
        }

        if (is_hw(target_fmt)){
            if (target_fmt == AV_PIX_FMT_VAAPI){
                filters_txt << ":derive_device=vaapi";
            } else if (target_fmt == AV_PIX_FMT_OPENCL){
                filters_txt << ":derive_device=opencl";
            } else {//check if we made a mistake, VDPAU is not a HW output format
                LWARN("Unknown target HW format");
            }
        }
        filters_txt << ",format=pix_fmts=" << target_fmt;
    } else {
        //SW conversion
        filters_txt << "format=pix_fmts=" << target_fmt;
    }

    LDEBUG("Filter str: " + filters_txt.str());
    //FIXME: Take user inputs
    return filters_txt.str();
}

void Filter::set_source_time_base(const AVRational tb) {
    time_base = tb;
    //nothing...
};
