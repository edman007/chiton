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
 *   Copyright 2020-2021 Ed Martin <edman007@edman007.com>
 *
 **************************************************************************
 */
#include "stream_unwrap.hpp"
#include "util.hpp"
#include "chiton_ffmpeg.hpp"


StreamUnwrap::StreamUnwrap(Config& cfg) : cfg(cfg) {
    input_format_context = NULL;
    max_sync_offset = cfg.get_value_int("max-sync-offset");
    LINFO("Unwrap is loaded" + cfg.get_value("camera-id"));
}

StreamUnwrap::~StreamUnwrap(){
    close();
    for (auto &ctx : decode_ctx){
        avcodec_free_context(&ctx.second);
    }
}

bool StreamUnwrap::close(void){
    //the reorder queue should be freed
    for (auto pkt : reorder_queue){
        unref_frame(pkt);
    }
    reorder_queue.clear();
    reorder_len = 0;
    
    avformat_close_input(&input_format_context);
    input_format_context = NULL;
    return true;
}

bool StreamUnwrap::connect(void) {
    LINFO("Reorder queue was first set at " + cfg.get_value("reorder-queue-len"));
    const std::string& url = cfg.get_value("video-url");
    if (!url.compare("")){
        LERROR( "Camera was not supplied with a URL" + url);
        return false;
    }


    int error;
    AVDictionary *opts = get_options();
#ifdef DEBUG
    /* Open the input file to read from it. */
    if (av_dict_count(opts)){
        LWARN("Setting Options:");
        dump_options(opts);
    }
#endif

    if ((error = avformat_open_input(&input_format_context, url.c_str(), NULL, &opts)) < 0) {
        LERROR( "Could not open camera url '" + url + "' (error '" + std::string(av_err2str(error)) +")");
        input_format_context = NULL;
        av_dict_free(&opts);
        return false;
    }

    
    Util::get_videotime(connect_time);

#ifdef DEBUG
    if (av_dict_count(opts)){
        LWARN("Invalid Options:");
        dump_options(opts);
    }
#endif

    av_dict_free(&opts);
    
    /* Get information on the input file (number of streams etc.). */
    if ((error = avformat_find_stream_info(input_format_context, NULL)) < 0) {
        LERROR( "Could not open find stream info (error '" + std::string(av_err2str(error)) + "')");
        avformat_close_input(&input_format_context);
        return false;
    }
    LINFO("Video Stream has " + std::to_string(input_format_context->nb_streams) + " streams");

#ifdef DEBUG
    //print all the stream info
    for (unsigned int i = 0; i < input_format_context->nb_streams; i++){
        LINFO("Stream " + std::to_string(i) + " details, codec " + std::to_string(input_format_context->streams[i]->codecpar->codec_type)+ ":");
        av_dump_format(input_format_context, i, url.c_str(), 0);
    }
#endif

    input_format_context->flags |= AVFMT_FLAG_GENPTS;//fix the timestamps...
    
    
    //start reading frames
    reorder_len = cfg.get_value_int("reorder-queue-len");
    LINFO("Reorder queue was set at " + cfg.get_value("reorder-queue-len"));
    return charge_reorder_queue();
}

AVFormatContext* StreamUnwrap::get_format_context(void){
    return input_format_context;
}

unsigned int StreamUnwrap::get_stream_count(void){
    return input_format_context->nb_streams;
}

bool StreamUnwrap::alloc_decode_context(unsigned int stream){
    AVCodecContext *avctx;
    AVCodec *input_codec;
    int error;
    if (decode_ctx.find(stream) != decode_ctx.end()){
        LDEBUG("Not generating decode context, it already exists");
        return true;//already exists, bail early
    }

    if (stream >= get_stream_count()){
        LERROR("Attempted to access stream that doesn't exist");
        return false;
    }

    //locate a decoder for the codec
    input_codec = avcodec_find_decoder(input_format_context->streams[stream]->codecpar->codec_id);
    if (!input_codec){
        LWARN("Could not find decoder");
        return false;
    }

    /* Allocate a new decoding context. */
    avctx = avcodec_alloc_context3(input_codec);
    if (!avctx) {
        LERROR("Could not allocate a decoding context");
        return false;
    }
    
    /* Initialize the stream parameters with demuxer information. */
    error = avcodec_parameters_to_context(avctx, input_format_context->streams[stream]->codecpar);
    if (error < 0) {
        avcodec_free_context(&avctx);
        return false;
    }
    
    /* Open the decoder. */
    global_codec_lock.lock();
    if ((error = avcodec_open2(avctx, input_codec, NULL)) < 0) {
        global_codec_lock.unlock();
        LERROR("Could not open input codec (error '" + std::string(av_err2str(error)) + "')");
        avcodec_free_context(&avctx);
        return false;
    }
    global_codec_lock.unlock();
    
    /* Save the decoder context for easier access later. */
    decode_ctx[stream] = avctx;
    return true;

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


AVDictionary *StreamUnwrap::get_options(void){
    AVDictionary *dict = NULL;
    if (cfg.get_value("ffmpeg-demux-options") == ""){
        return NULL;
    }
    int error;
    if ((error = av_dict_parse_string(&dict, cfg.get_value("ffmpeg-demux-options").c_str(), "=", ":", 0))){
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
        if (!read_frame()){
            break;//error reading a frame we exit
        }
    }
    return true;
}

void StreamUnwrap::sort_reorder_queue(void){
    if (reorder_queue.size() < 2){
        //nothing to sort if it's this small
        return;
    }
    auto av_old_back = reorder_queue.end();
    std::advance(av_old_back, -2);
    AVPacket &old_back = *av_old_back;
    long back_dts_in_old_back = av_rescale_q_rnd(reorder_queue.back().dts,
                                                 input_format_context->streams[reorder_queue.back().stream_index]->time_base,
                                                 input_format_context->streams[old_back.stream_index]->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
    
    
    if (old_back.dts > back_dts_in_old_back){
        //need to walk the queue and insert it in the right spot...
        bool back_is_video = input_format_context->streams[reorder_queue.back().stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO;
        //FIXME: this loop should be optimized to run back to front, as that's the short direction
        for (std::list<AVPacket>::iterator itr = reorder_queue.begin(); itr != reorder_queue.end() ; itr++){
            long back_dts_in_itr = av_rescale_q_rnd(reorder_queue.back().dts,
                                                    input_format_context->streams[reorder_queue.back().stream_index]->time_base,
                                                    input_format_context->streams[itr->stream_index]->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
            if ((itr->dts > reorder_queue.back().dts && itr->stream_index == reorder_queue.back().stream_index) || //same stream and dts is out of order
                (itr->stream_index != reorder_queue.back().stream_index && itr->dts > back_dts_in_itr) || // different stream and out of order
                (itr->stream_index != reorder_queue.back().stream_index && itr->dts == back_dts_in_itr && back_is_video)){ //different stream and video and same time
                if (itr == reorder_queue.begin() && reorder_queue.size() == reorder_len){
                    //itr is the front here
                    //estimate what it would take
                    long dts_itr_to_front = itr->dts - back_dts_in_itr;//this is how far ahead the packet was from the queue
                    auto av_back = reorder_queue.end();
                    std::advance(av_back, -2);
                    AVPacket &back = *av_back;
                    
                    AVPacket &front = reorder_queue.front();
                    long back_dts_in_itr = av_rescale_q_rnd(back.dts,
                                                            input_format_context->streams[back.stream_index]->time_base,
                                                            input_format_context->streams[itr->stream_index]->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
                    long front_dts_in_itr = av_rescale_q_rnd(front.dts,
                                                             input_format_context->streams[front.stream_index]->time_base,
                                                             input_format_context->streams[itr->stream_index]->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
                    double que_lent = av_q2d(input_format_context->streams[itr->stream_index]->time_base) * ( back_dts_in_itr - front_dts_in_itr);
                    double target_addational_lent = av_q2d(input_format_context->streams[itr->stream_index]->time_base) * ( dts_itr_to_front );
                    double target_change = (target_addational_lent/que_lent) + 1.0;
                    LINFO("Reorder queue is short by " + std::to_string(target_addational_lent) + "s, recommend making reorder-queue-len at least " +
                          std::to_string((int)(target_change * reorder_queue.size())) + " for camera " /*+ cfg.get_value("camera-id") */);
                    
                    
                }
                reorder_queue.splice(itr, reorder_queue, std::prev(reorder_queue.end()));
                break;
            }
        }
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

    struct timeval recv_time;
    Util::get_videotime(recv_time);
    recv_time.tv_sec -= connect_time.tv_sec;
    
    sort_reorder_queue();

    //check if this is a file, if yes we skip timestamp resyncing
    if (input_format_context->url && input_format_context->url[0] == '/'){
        return true;
    }
    //check the time of the youngest packet...
    double delta = recv_time.tv_sec;
    delta -= av_q2d(input_format_context->streams[reorder_queue.back().stream_index]->time_base) * reorder_queue.back().dts;
    if (delta > max_sync_offset || delta < -max_sync_offset){
        connect_time.tv_sec += delta;
        LWARN("Input stream has drifted " + std::to_string(delta) + "s from wall time on camera " + cfg.get_value("camera-id") + ", resyncing..." );
    }
    
    return true;
}

const struct timeval& StreamUnwrap::get_start_time(void){
    return connect_time;
}

bool StreamUnwrap::decode_packet(AVPacket &packet){
    int ret;
    if (decode_ctx.find(packet.stream_index) == decode_ctx.end()){
        if (!alloc_decode_context(packet.stream_index)){
            return false;
        }
    }

    ret = avcodec_send_packet(decode_ctx[packet.stream_index], &packet);
    if (ret == AVERROR(EAGAIN)){
        //probably should return ret instead of this as it's a soft error
        LWARN("Attempted to decode data without reading all frames");
        return false;
    } else if (ret == AVERROR(EINVAL)){
        return false;
        LWARN("Codec error: needs a flush?");
    } else if (ret == AVERROR(ENOMEM)){
        //probably need to think about restarting the camera
        LWARN("Error Decoding Packet, buffer is full");
        return false;
    } else if (ret < 0){
        //probably should restart the camera
        LWARN("Unknown decoding error");
        return false;
    }
    return true;
}

bool StreamUnwrap::get_decoded_frame(int stream, AVFrame *frame){
    if (decode_ctx.find(stream) == decode_ctx.end()){
        LWARN("Tried to decode a frame without a decoder!");
        return false;
    }
    int ret = avcodec_receive_frame(decode_ctx[stream], frame);
    if (ret == 0){
        return true;
    } else if (ret == AVERROR(EAGAIN)){
        //need to call decode packet
        return false;
    } else {
        //probably need to think about restarting the camera
        LWARN("Error reading frame");
        return false;
    }
}

void StreamUnwrap::timestamp(const AVPacket &packet, struct timeval &time){
    Util::compute_timestamp(get_start_time(), time, packet.pts, input_format_context->streams[packet.stream_index]->time_base);
}

bool StreamUnwrap::is_audio(const AVPacket &packet){
    return input_format_context->streams[packet.stream_index]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO;
}

bool StreamUnwrap::is_video(const AVPacket &packet){
    return input_format_context->streams[packet.stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO;
}

AVStream *StreamUnwrap::get_stream(const AVPacket &packet){
    return get_stream(packet.stream_index);
}

AVStream *StreamUnwrap::get_stream(const int id){
    return input_format_context->streams[id];
}

AVStream *StreamUnwrap::get_audio_stream(void){
    int best_stream = av_find_best_stream(input_format_context, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (best_stream == AVERROR_STREAM_NOT_FOUND || best_stream == AVERROR_DECODER_NOT_FOUND){
        return NULL;
    }
    return get_stream(best_stream);
}

AVStream *StreamUnwrap::get_video_stream(void){
    int best_stream = av_find_best_stream(input_format_context, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (best_stream == AVERROR_STREAM_NOT_FOUND || best_stream == AVERROR_DECODER_NOT_FOUND){
        return NULL;
    }
    return get_stream(best_stream);
}

AVCodecContext* StreamUnwrap::get_codec_context(AVStream *stream){
    if (!stream){
        return NULL;
    }

    if (decode_ctx.find(stream->index) == decode_ctx.end()){
        if (!alloc_decode_context(stream->index)){
            return NULL;
        }
    }

    return decode_ctx[stream->index];
}
