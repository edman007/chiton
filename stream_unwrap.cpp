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
    max_dts = 0;
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
    AVDictionary *opts = get_options();
    /* Open the input file to read from it. */
    if (av_dict_count(opts)){
        LWARN("Setting Options:");
        dump_options(opts);
    }
    if ((error = avformat_open_input(&input_format_context, url.c_str(), NULL, &opts)) < 0) {
        Util::log_msg(LOG_ERROR, "Could not open camera url '" + url + "' (error '" + std::string(av_err2str(error)) +")");
        input_format_context = NULL;
        av_dict_free(&opts);
        return false;
    }
    if (av_dict_count(opts)){
        LWARN("Invalid Options:");
        dump_options(opts);
    }
    av_dict_free(&opts);
    
    /* Get information on the input file (number of streams etc.). */
    if ((error = avformat_find_stream_info(input_format_context, NULL)) < 0) {
        Util::log_msg(LOG_ERROR, "Could not open find stream info (error '" + std::string(av_err2str(error)) + "')");
        avformat_close_input(&input_format_context);
        return false;
    }
    LINFO("Video Stream has " + std::to_string(input_format_context->nb_streams) + " streams");
    input_format_context->flags |= AVFMT_FLAG_GENPTS;//fix the timestamps...    
    /*
    if (!(input_codec = avcodec_find_decoder(input_format_context->streams[0]->codecpar->codec_id))) {
        LERROR("Could not find input codec\n");
        avformat_close_input(input_format_context);
        return false;
    }
    */
    //start reading frames
    reorder_len = cfg.get_value_int("reorder_queue_len");
    return charge_reorder_queue();
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
        LERROR("Could not allocate a decoding context");
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
    read_frame();
    if (reorder_queue.size() > 0){
        av_packet_move_ref(&packet, &reorder_queue.front());
        reorder_queue.pop_front();
        return true;
    } else {
        return false;
    }
    

}

void StreamUnwrap::unref_frame(AVPacket &packet){
    av_packet_unref(&packet);
}

//reorder_queue_size = 500
AVDictionary *StreamUnwrap::get_options(void){
    AVDictionary *dict = NULL;
    if (cfg.get_value("ffmpeg-demux-options") == ""){
        return NULL;
    }
    int error;
    if (error = av_dict_parse_string(&dict, cfg.get_value("ffmpeg-demux-options").c_str(), "=", ":", 0)){
        LERROR("Error Parsing input ffmpeg options: " + std::string(av_err2str(error)));
        av_dict_free(&dict);
        return NULL;
    }
    return dict;
}

void StreamUnwrap::dump_options(AVDictionary* dict){
    AVDictionaryEntry *en = NULL;
    while ((en = av_dict_get(dict, "", en, AV_DICT_IGNORE_SUFFIX))) {
        LWARN("\t" + std::string(en->key) + "=" + std::string(en->value));
    }
}

bool StreamUnwrap::charge_reorder_queue(void){
    while (reorder_queue.size() < reorder_len){
        read_frame();
    }
    return true;
}

void StreamUnwrap::sort_reorder_queue(void){
    if (reorder_queue.back().dts < max_dts){
        //need to walk the queue and insert it in the right spot...
        for (std::list<AVPacket>::iterator itr = reorder_queue.begin(); itr != reorder_queue.end() ; itr++){
            if (itr->dts > reorder_queue.back().dts){
                reorder_queue.splice(itr, reorder_queue, std::prev(reorder_queue.end()));
                break;
            }
        }
    } else {
        max_dts = reorder_queue.back().dts;
    }

}

bool StreamUnwrap::read_frame(void){
    reorder_queue.emplace_back();
    if (av_read_frame(input_format_context, &reorder_queue.back())){
        reorder_queue.pop_back();
        return false;
    }
    //fixup this data...maybe limit it to the first one?
    if (reorder_queue.back().dts == AV_NOPTS_VALUE){
        reorder_queue.back().dts = 0;
    }
    if (reorder_queue.back().pts == AV_NOPTS_VALUE){
        reorder_queue.back().pts = 0;
    }

    sort_reorder_queue();
    return true;
}
