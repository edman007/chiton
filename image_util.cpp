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

#include "image_util.hpp"
#include "util.hpp"
#include "file_manager.hpp"

ImageUtil::ImageUtil(Database &db, Config &cfg) : db(db), cfg(cfg) {

}

ImageUtil::~ImageUtil(){

}

bool ImageUtil::write_frame_jpg(const AVFrame *frame, std::string &name, const struct timeval *start_time /* = NULL */, rect src/* = {-1, -1, 0, 0}*/){
    LDEBUG("Writing JPEG");
    //check if it's valid
    if (!frame){
        LWARN("Cannot write frame out as image, invalid format");
        return false;
    }

    AVFrame *cropped_frame = apply_rect(frame, src);

    AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    if (!codec) {
        LWARN("Cannot find MJPEG encoder!");
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
    c->pix_fmt = AV_PIX_FMT_YUVJ420P;
    LINFO("Exporting JPEG " + std::to_string(c->width) + "x" + std::to_string(c->height));

    global_codec_lock.lock();
    if (avcodec_open2(c, codec, NULL) < 0){
        global_codec_lock.unlock();
        avcodec_free_context(&c);
        LWARN("Failed to open codec");
        return false;
    }
    global_codec_lock.unlock();

    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    int ret = avcodec_send_frame(c, cropped_frame);
    if (ret){
        //failed...free and exit
        LWARN("Failed to Send Frame for encoding");
        avcodec_free_context(&c);
        av_packet_unref(&pkt);
        av_frame_free(&cropped_frame);
        return false;
    }

    ret = avcodec_receive_packet(c, &pkt);
    avcodec_free_context(&c);
    av_frame_free(&cropped_frame);
    if (ret){
        LWARN("Failed to receive decoded frame");
        //failed...free and exit
        av_packet_unref(&pkt);
        return false;
    }

    //write out the packet
    FileManager fm(db, cfg);
    std::string path;
    bool success = false;
    if (fm.get_image_path(path, name, ".jpg", start_time)){
        std::ofstream out_s = fm.get_fstream_write(name, path);
        LWARN("Writing to " + name);
        if (out_s.is_open()){
            out_s.write(reinterpret_cast<char*>(pkt.data), pkt.size);
            success = out_s.good();
            out_s.close();
        } else {
            LWARN("Couldn't open JPG");
        }
    } else {
        LWARN("Failed to get image path");
    }
    av_packet_unref(&pkt);

    return success;
}

bool ImageUtil::write_frame_png(const AVFrame *frame, std::string &name, const struct timeval *start_time /* = NULL */, rect src/* = {-1, -1, 0, 0}*/){
    LWARN("Can't write out PNGs");
    return false;
}

AVFrame* ImageUtil::apply_rect(const AVFrame *frame, rect &src){
        //copy the frame
    AVFrame* out = av_frame_clone(frame);

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
        out->crop_bottom = src.y;
        out->crop_right = out->width - out->crop_left - src.w;
        out->crop_top = out->height - out->crop_bottom - src.h;
    }

    return out;
}
