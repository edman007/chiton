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
 *   Copyright 2021-2023 Ed Martin <edman007@edman007.com>
 *
 **************************************************************************
 */

#include "image_util.hpp"
#include "util.hpp"
#include "file_manager.hpp"
#include "filter.hpp"
#include "system_controller.hpp"

ImageUtil::ImageUtil(SystemController &sys, Config &cfg) : sys(sys), db(sys.get_db()), cfg(cfg) {
    codec_id = AV_CODEC_ID_NONE;
    codec_profile = FF_PROFILE_UNKNOWN;
    pkt = av_packet_alloc();
}

ImageUtil::~ImageUtil(){
    av_packet_free(&pkt);
}

void ImageUtil::set_profile(AVCodecID id, int profile){
    codec_id = id;
    codec_profile = profile;
}

bool ImageUtil::write_frame_jpg(const AVFrame *frame, std::string &name, const struct timeval *start_time /* = NULL */, rect src/* = {-1, -1, 0, 0}*/,  long *file_id/* = NULL*/){
    LDEBUG("Writing JPEG");
    //check if it's valid
    if (!frame){
        LWARN("Cannot write frame out as image, invalid format");
        return false;
    }

    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    if (!codec) {
        LWARN("Cannot find MJPEG encoder!");
        return false;
    }


    AVFrame *cropped_frame = NULL;
    if (format_supported(frame, codec)){
        LDEBUG("Format is supported" + std::to_string(frame->format));
        cropped_frame = apply_rect(frame, src);
    } else {
        //run it through a single loop of the filter
        LDEBUG("Filtering prior to writing");
        Filter flt(cfg);
        flt.set_target_fmt(get_pix_mpjeg_fmt(codec->pix_fmts), AV_CODEC_ID_MJPEG, FF_PROFILE_MJPEG_HUFFMAN_BASELINE_DCT);
        flt.set_source_time_base({1, 1});//I don't think it actually matters...
        flt.set_source_codec(codec_id, codec_profile);
        flt.send_frame(frame);
        AVFrame *filtered_frame = av_frame_alloc();
        flt.get_frame(filtered_frame);
        cropped_frame = apply_rect(filtered_frame, src);
        av_frame_free(&filtered_frame);
    }

    if (!cropped_frame){
        LWARN("Failed to get cropped frame, not writing image");
        return false;
    }

    AVCodecContext* c = avcodec_alloc_context3(codec);
    if (!c) {
        LWARN("Could not allocate video codec context");
        return false;
    }


    //should  do avcodec_parameters_to_context or something?
    c->bit_rate = 400000;
    c->width = cropped_frame->width - cropped_frame->crop_left - cropped_frame->crop_right;
    c->height = cropped_frame->height - cropped_frame->crop_top - cropped_frame->crop_bottom;
    c->time_base = (AVRational){1,1};
    c->pix_fmt = static_cast<enum AVPixelFormat>(cropped_frame->format);
    c->sample_aspect_ratio = frame->sample_aspect_ratio;
    c->strict_std_compliance = FF_COMPLIANCE_UNOFFICIAL;//new ffmpeg complains about our YUV range, this makes it happy enough
    LINFO("Exporting JPEG " + std::to_string(c->width) + "x" + std::to_string(c->height));

    gcff_util.lock();
    if (avcodec_open2(c, codec, NULL) < 0){
        gcff_util.unlock();
        avcodec_free_context(&c);
        LWARN("Failed to open MJPEG codec");
        return false;
    }
    gcff_util.unlock();

    int ret = avcodec_send_frame(c, cropped_frame);
    if (ret){
        //failed...free and exit
        LWARN("Failed to Send Frame for encoding");
        avcodec_free_context(&c);
        av_packet_unref(pkt);
        av_frame_free(&cropped_frame);
        return false;
    }

    ret = avcodec_receive_packet(c, pkt);
    avcodec_free_context(&c);
    av_frame_free(&cropped_frame);
    if (ret){
        LWARN("Failed to receive decoded frame");
        //failed...free and exit
        av_packet_unref(pkt);
        return false;
    }

    //write out the packet
    FileManager fm(sys, cfg);
    std::string path;
    bool success = false;
    if (fm.get_image_path(path, name, ".jpg", start_time, file_id)){
        std::ofstream out_s = fm.get_fstream_write(name, path);
        LWARN("Writing to " + name);
        if (out_s.is_open()){
            out_s.write(reinterpret_cast<char*>(pkt->data), pkt->size);
            success = out_s.good();
            out_s.close();
        } else {
            LWARN("Couldn't open JPG");
        }
    } else {
        LWARN("Failed to get image path");
    }
    av_packet_unref(pkt);

    return success;
}

bool ImageUtil::write_frame_png(const AVFrame *frame, std::string &name, const struct timeval *start_time /* = NULL */, rect src/* = {-1, -1, 0, 0}*/,  long *file_id/* = NULL*/){
    LWARN("Can't write out PNGs");
    return false;
}

AVFrame* ImageUtil::apply_rect(const AVFrame *frame, rect &src){
    //copy the frame
    AVFrame* out = av_frame_clone(frame);
    if (!out){
        LWARN("av_frame_clone() failed");
        return NULL;
    }

    //adjust rect and apply if x is not negative
    if (src.x >= 0){
        //correct any numbers
        if (static_cast<unsigned int>(src.x) < frame->crop_left){
            src.x = frame->crop_left;
        }
        if (static_cast<unsigned int>(src.y) < frame->crop_bottom){
            src.y = frame->crop_bottom;
        }
        if (static_cast<unsigned int>(src.w) > (frame->width - src.x - frame->crop_right)){
            src.w = frame->width - src.x - frame->crop_right;
        }
        if (static_cast<unsigned int>(src.h) > (frame->height - src.y - frame->crop_top)){
            src.h = frame->height - src.y - frame->crop_top;
        }

        //and apply the new numbers
        out->crop_left = src.x;
        out->crop_top = src.y;
        out->crop_right = out->width - out->crop_left - src.w;
        out->crop_bottom = out->height - out->crop_top - src.h;
        if (av_frame_apply_cropping(out, AV_FRAME_CROP_UNALIGNED) < 0){
            LWARN("av_frame_apply_cropping returned an error");
        }
    }

    return out;
}

bool ImageUtil::format_supported(const AVFrame *frame, const AVCodec *codec){

    if (!codec->pix_fmts){
        return true;//is this right? it's actually unknown...but I guess the filter won't help
    }
    for (const enum AVPixelFormat *p = codec->pix_fmts; *p != AV_PIX_FMT_NONE; p++){
        if (*p == static_cast<const enum AVPixelFormat>(frame->format)){
            return true;
        }
    }

    return false;
}

AVPixelFormat ImageUtil::get_pix_mpjeg_fmt(const AVPixelFormat *pix_list){
    for (int i = 0; pix_list[i] != AV_PIX_FMT_NONE; i++){
        //switch is based on libswscale depracated pix format check for jpeg in utils.c
        switch (pix_list[i]) {
            //these are deprecated
        case AV_PIX_FMT_YUVJ420P:
        case AV_PIX_FMT_YUVJ411P:
        case AV_PIX_FMT_YUVJ422P:
        case AV_PIX_FMT_YUVJ444P:
        case AV_PIX_FMT_YUVJ440P:
            continue;
        default:
            return pix_list[i];
        }
    }
    return pix_list[0];//all of them were deprecated, return the first
}
