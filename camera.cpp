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

#include "camera.hpp"
#include "util.hpp"
#include "image_util.hpp"
#include "filter.hpp"
#include "motion_controller.hpp"

Camera::Camera(int camera, Database& db) : id(camera), db(db), stream(cfg), fm(db, cfg) {
    //load the config
    load_cfg();
    shutdown = false;
    alive = true;
    watchdog = false;
    startup = true;

    //variables for cutting files...
    last_cut = av_make_q(0, 1);
    last_cut_file = av_make_q(0, 1);
    last_cut_byte = 0;

    int seconds_per_segment_raw = cfg.get_value_int("seconds-per-segment");
    if (seconds_per_segment_raw <= 0){
        LWARN("seconds-per-segment was invalid");
        seconds_per_segment_raw = DEFAULT_SECONDS_PER_SEGMENT;
    }
    seconds_per_segment = av_make_q(seconds_per_segment_raw, 1);

    int seconds_per_file_raw = cfg.get_value_int("seconds-per-file");
    if (seconds_per_file_raw <= 0){
        LWARN("seconds-per-file was invalid");
        seconds_per_file_raw = 60*seconds_per_segment_raw;
    }
    seconds_per_file = av_make_q(seconds_per_file_raw, 1);
}
Camera::~Camera(){
    
}

void Camera::load_cfg(void){
    cfg.load_camera_config(id, db);
}
    
void Camera::run(void){
    if (cfg.get_value("display-name") == ""){
        Util::set_thread_name("Camera " + std::to_string(id), cfg);
    } else {
        Util::set_thread_name(cfg.get_value("display-name"), cfg);
    }
    LINFO("Camera " + std::to_string(id) + " starting...");
    if (!stream.connect()){
        alive = false;
        startup = false;
        return;
    }
    watchdog = true;
    startup = false;

    LINFO("Camera " + std::to_string(id) + " connected...");
    std::string out_filename = fm.get_next_path(file_id, id, stream.get_start_time());
    StreamWriter out = StreamWriter(cfg);
    MotionController motion(db, cfg, stream);
    out.change_path(out_filename);
    out.set_keyframe_callback(std::bind(&Camera::cut_video, this, std::placeholders::_1, std::placeholders::_2));
    //look at the stream and settings and see what needs encoding and decoding
    bool encode_video = get_vencode();
    bool encode_audio = get_aencode();

    //determine if motion detection wants to decode
    bool decode_video = encode_video || motion.decode_video();
    bool decode_audio = encode_audio || motion.decode_audio();

    LDEBUG("Encode/Decode: " + std::to_string(encode_video)+ std::to_string(encode_audio)+ std::to_string(decode_video)+ std::to_string(decode_audio));

    //allocate a frame (we only allocate when we want to decode, an unallocated frame means we skip decoding)
    AVFrame *frame = NULL, *filtered_frame = NULL;
    if (decode_video || decode_audio){
        frame = av_frame_alloc();
        if (!frame){
            LWARN("Failed to allocate frame");
            frame = NULL;
        }
        filtered_frame = av_frame_alloc();
        if (!filtered_frame){
            LWARN("Failed to allocate filtered frame");
            filtered_frame = NULL;
        }
    }

    Filter vfilter(cfg);//video filter, only initilized when encoding


    if (encode_video){
        LDEBUG("Charging Decoder");
        stream.charge_video_decoder();
        watchdog = true;
        AVStream *vstream = stream.get_video_stream();
        if (!vstream){
            LERROR("No Video stream found");
            return;
        }
        //init the filter with the decoded video frame
        stream.peek_decoded_vframe(frame);
        AVCodecID codec_id;
        int codec_profile;
        AVPixelFormat pix_fmt;
        out.get_video_format(frame, pix_fmt, codec_id, codec_profile);
        vfilter.set_target_fmt(pix_fmt, codec_id, codec_profile);
        vfilter.set_source_time_base(vstream->time_base);
        vfilter.send_frame(frame);
        av_frame_unref(frame);
        vfilter.peek_frame(filtered_frame);
        AVCodecContext *vctx = stream.get_codec_context(vstream);
        bool encoded_ret = out.add_encoded_stream(vstream, vctx, filtered_frame);
        av_frame_unref(filtered_frame);
        if (encoded_ret){
            LINFO("Camera " + std::to_string(id) + " transcoding video stream");
        } else {
            LERROR("Camera " + std::to_string(id) + " transcoding video stream failed!");
            return;
        }
    } else {
        if (out.add_stream(stream.get_video_stream())){
            LINFO("Camera " + std::to_string(id) + " copying video stream");
        } else {
            LERROR("Camera " + std::to_string(id) + " copying video stream failed!");
            return;
        }
    }

    if (encode_audio){
        AVStream *astream = stream.get_audio_stream();
        AVCodecContext *actx = stream.get_codec_context(astream);
        if (out.add_encoded_stream(astream, actx)){
            LINFO("Camera " + std::to_string(id) + " transcoding audio stream");
        } else {
            LERROR("Camera " + std::to_string(id) + " transcoding audio stream failed!");
            return;
        }
    } else if(cfg.get_value("encode-format-audio") != "none") {
        if (out.add_stream(stream.get_audio_stream())){
            LINFO("Camera " + std::to_string(id) + " copying audio stream");
        } else {
            LINFO("Camera " + std::to_string(id) + " copying audio stream failed!");
        }
    }

    out.open();
    
    AVPacket pkt;
    bool valid_keyframe = false;

    //used for calculating shutdown time for the last segment
    long last_pts = 0;
    long last_stream_index = 0;
    bool failed = false;
    int frame_cnt = 0;
    ImageUtil img(db, cfg);
    motion.set_streams();
    while (!shutdown && !failed && stream.get_next_frame(pkt)){
        watchdog = true;
        last_pts = pkt.pts;
        last_stream_index = pkt.stream_index;
        frame_cnt++;

        //we skip processing until we get a video keyframe
        if (valid_keyframe || (pkt.flags & AV_PKT_FLAG_KEY && stream.is_video(pkt))){
            valid_keyframe = true;
        } else {
            LINFO("Dropping packets before first keyframe");
            stream.unref_frame(pkt);
            continue;
        }

        //decode the packets
        if (frame && stream.is_video(pkt) && decode_video){
            if (stream.decode_packet(pkt)){
                while (!failed && stream.get_decoded_frame(pkt.stream_index, frame)){
                    //LDEBUG("Decoded Video Frame");
                    motion.process_frame(pkt.stream_index, frame);
                    if (encode_video){
                        //Filter the frame before encoding
                        if (vfilter.send_frame(frame)){
                            while (!failed && vfilter.get_frame(filtered_frame)){
                                /*
                                if (frame_cnt == 10){//debug stuff
                                    std::string debug_frame_name = "debug_frame";
                                    img.write_frame_jpg(filtered_frame, debug_frame_name);
                                    LINFO("Wrote " + debug_frame_name);
                                }
                                */
                                if (!out.write(filtered_frame, stream.get_stream(pkt))){
                                    LERROR("Error Encoding Video!");
                                    failed = true;//error encoding
                                }
                                av_frame_unref(filtered_frame);
                            }
                        } else {
                            LERROR("Failed to send frame to video filter");
                            stream.unref_frame(pkt);
                            failed = true;
                        }
                    }
                }
            }
            if (!encode_video){
                if (!out.write(pkt, stream.get_stream(pkt))){//log it
                    LERROR("Error writing copied video packet");
                    failed = true;
                }
            }
        } else if (frame && stream.is_audio(pkt) && decode_audio){
                if (stream.decode_packet(pkt)){
                    while (!failed && stream.get_decoded_frame(pkt.stream_index, frame)){
                        //LDEBUG("Decoded Audio Frame");
                        motion.process_frame(pkt.stream_index, frame);
                        if (encode_audio){
                            if (!out.write(frame, stream.get_stream(pkt))){
                                stream.unref_frame(pkt);
                                LERROR("Error Encoding Audio!");
                                failed = true;
                            }
                        }
                    }
                }
                if (!encode_audio){
                    if (!out.write(pkt, stream.get_stream(pkt))){//log it
                        stream.unref_frame(pkt);
                        LERROR("Error writing copied audio packet");
                        failed = true;
                    }
                }
        } else {
            //this packet is being copied...
            if (!out.write(pkt, stream.get_stream(pkt))){//log it
                LERROR("Error writing packet!");
                failed = true;
            }
        }
        
        stream.unref_frame(pkt);
    }
    LINFO("Camera " + std::to_string(id)+ " is exiting");

    struct timeval end;
    Util::compute_timestamp(stream.get_start_time(), end, last_pts, stream.get_format_context()->streams[last_stream_index]->time_base);
    long long size = out.close();
    fm.update_file_metadata(file_id, end, size, out, last_cut_byte);

    if (frame){
        av_frame_free(&frame);
        frame = NULL;
    }

    alive = false;
}

void Camera::stop(void){
    shutdown = true;
}

bool Camera::ping(void){
    return !watchdog.exchange(false) || !alive;
}

int Camera::get_id(void){
    return id;
}

bool Camera::in_startup(void){
    return startup && alive;
}


void Camera::set_thread_id(std::thread::id tid){
    thread_id = tid;
}
std::thread::id Camera::get_thread_id(void){
    return thread_id;
}

void Camera::cut_video(const AVPacket &pkt, StreamWriter &out){
    if (pkt.flags & AV_PKT_FLAG_KEY && stream.is_video(pkt)){
        //calculate the seconds:
        AVRational sec_raw = av_mul_q(av_make_q(pkt.dts, 1), stream.get_format_context()->streams[pkt.stream_index]->time_base);//current time..
        AVRational sec = av_sub_q(sec_raw, last_cut);

        if (av_cmp_q(sec, seconds_per_segment) == 1 || (sec.num < 0 && last_cut.num > 0)){
            //cutting the video
            struct timeval start;
            stream.timestamp(pkt, start);
            AVRational file_seconds = av_sub_q(sec_raw, last_cut_file);
            if (out.is_fragmented() && sec.num >= 0 && av_cmp_q(file_seconds, seconds_per_file) == -1){
                LDEBUG("Fragging Segment");
                //we just fragment the file
                long long size = out.change_path("");
                if (last_cut_byte == 0 && out.get_init_len() > 0){
                    last_cut_byte = out.get_init_len();
                }
                fm.update_file_metadata(file_id, start, size, out, last_cut_byte);
                last_cut_byte = size;
                fm.get_next_path(file_id, id, start, true);
            } else {
                LDEBUG("Cutting File");
                //copy the previous metadata
                auto old_id = file_id;
                auto old_init_len = out.get_init_len();
                auto end = start;
                if (sec.num < 0){
                    //this will cause a discontinuity which will be picked up and cause it to play correctly
                    start.tv_usec += 1000;
                }
                std::string out_filename = fm.get_next_path(file_id, id, start);
                long long size = out.change_path(out_filename);;//apply new filename, and get length of old file
                fm.update_file_metadata(old_id, end, size, out, last_cut_byte, old_init_len);
                last_cut_byte = 0;
                last_cut_file = av_mul_q(av_make_q(pkt.dts, 1), stream.get_format_context()->streams[pkt.stream_index]->time_base);
            }
            //save out this position
            last_cut = av_mul_q(av_make_q(pkt.dts, 1), stream.get_format_context()->streams[pkt.stream_index]->time_base);
        }
    }
}

bool Camera::get_vencode(void){
    AVStream *video_stream = stream.get_video_stream();
    if (video_stream == NULL){
        LERROR("Camera " + std::to_string(id) + " has no usable video, this will probably not work");
        return false;
    }

    const std::string &user_opt = cfg.get_value("encode-format-video");
    if (user_opt == "h264" || user_opt == "hevc"){
        return true;//these force encoding
    } else {
        if (user_opt != "copy"){
            LWARN("encode-format-video is '" + user_opt + "' which is not valid, on camera " + std::to_string(id));
        }
        if (video_stream->codecpar->codec_id == AV_CODEC_ID_H264 || video_stream->codecpar->codec_id == AV_CODEC_ID_HEVC){
            return false;
        } else {
            LINFO("Transcoding audio because camera provided unsupported format");
            return true;//there is video, but it's not HLS compatible (MJPEG or something?)
        }
    }
}

bool Camera::get_aencode(void){
    const std::string &user_opt = cfg.get_value("encode-format-audio");
    if (user_opt == "none"){
        LINFO("encode-format-audio is 'none', skipping audio");
        return false;
    }

    AVStream *audio_stream = stream.get_audio_stream();
    if (audio_stream == NULL){
        LINFO("Camera " + std::to_string(id) + " has no usable audio");
        return false;
    }

    if (user_opt == "aac" || user_opt == "ac3"){
        return true;//these force encoding
    } else {
        if (user_opt != "copy"){
            LWARN("encode-format-audio is '" + user_opt + "' which is not valid, on camera " + std::to_string(id));
        }
        if (audio_stream->codecpar->codec_id == AV_CODEC_ID_AAC || audio_stream->codecpar->codec_id == AV_CODEC_ID_AC3){
            return false;
        } else {
            LINFO("Transcoding audio because camera provided unsupported format");
            return true;//there is audio, but it's not HLS compatible
        }
    }

}
