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
 *   Copyright 2020 Ed Martin <edman007@edman007.com>
 *
 **************************************************************************
 */
#include "stream_unwrap.hpp"
#include "util.hpp"
#include "chiton_ffmpeg.hpp"


StreamUnwrap::StreamUnwrap(Config& cfg) : cfg(cfg) {
    input_format_context = NULL;

}

StreamUnwrap::~StreamUnwrap(){
    avformat_close_input(&input_format_context);

}

bool StreamUnwrap::connect(void) {
    const std::string& url = cfg.get_value("video-url");
    if (!url.compare("")){
        Util::log_msg(LOG_ERROR, "Camera was not supplied with a URL" + url);
        return false;
    }


    int error;
    /* Open the input file to read from it. */
    if ((error = avformat_open_input(&input_format_context, url.c_str(), NULL, NULL)) < 0) {
        Util::log_msg(LOG_ERROR, "Could not open camera url '" + url + "' (error '" + std::string(av_err2str(error)) +")");
        input_format_context = NULL;
        return false;
    }

    /* Get information on the input file (number of streams etc.). */
    if ((error = avformat_find_stream_info(input_format_context, NULL)) < 0) {
        Util::log_msg(LOG_ERROR, "Could not open find stream info (error '" + std::string(av_err2str(error)) + "')");
        avformat_close_input(&input_format_context);
        return false;
    }
    LINFO("Video Stream has " + std::to_string(input_format_context->nb_streams) + " streams");
    
    /*
    if (!(input_codec = avcodec_find_decoder(input_format_context->streams[0]->codecpar->codec_id))) {
        LERROR("Could not find input codec\n");
        avformat_close_input(input_format_context);
        return false;
    }
    */
    
    return true;
}

AVFormatContext* StreamUnwrap::get_format_context(void){
    return input_format_context;
}

unsigned int StreamUnwrap::get_stream_count(void){
    return input_format_context->nb_streams;
}

AVCodecContext* StreamUnwrap::alloc_decode_context(unsigned int stream){
    AVCodecContext *avctx;
    AVCodec *input_codec;
    int error;
    
    if (stream >= get_stream_count()){
        LERROR("Attempted to access stream that doesn't exist");
        return NULL;
    }
    
    /* Allocate a new decoding context. */
    avctx = avcodec_alloc_context3(input_codec);
    if (!avctx) {
        fprintf(stderr, "Could not allocate a decoding context");
        return NULL;
    }
    
    /* Initialize the stream parameters with demuxer information. */
    error = avcodec_parameters_to_context(avctx, input_format_context->streams[stream]->codecpar);
    if (error < 0) {
        avcodec_free_context(&avctx);
        return NULL;
    }
    
    /* Open the decoder. */
    if ((error = avcodec_open2(avctx, input_codec, NULL)) < 0) {
        LERROR("Could not open input codec (error '" + std::string(av_err2str(error)) + "')");
        avcodec_free_context(&avctx);
        return NULL;
    }
    
    /* Save the decoder context for easier access later. */
    return avctx;

}

bool StreamUnwrap::get_next_frame(AVPacket &packet){
    return 0 == av_read_frame(input_format_context, &packet);
}

void StreamUnwrap::unref_frame(AVPacket &packet){
    av_packet_unref(&packet);
}
