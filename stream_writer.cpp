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
#include "stream_writer.hpp"
#include "util.hpp"
#include "chiton_ffmpeg.hpp"
#include <assert.h>

StreamWriter::StreamWriter(Config& cfg) : cfg(cfg) {
    file_opened = false;
    init_seg = NULL;
    init_len = -1;
    output_file = NULL;
    if (cfg.get_value("output-extension") == ".mp4"){
        segment_mode = SEGMENT_FMP4;
    } else {
        segment_mode = SEGMENT_MPGTS;
    }
}

bool StreamWriter::open(void){
    if (file_opened){
        return true;//already opened
    }
    
    stream_offset.clear();
    last_dts.clear();

    return open_path();
}

bool StreamWriter::open_path(void){
    int error;

    AVOutputFormat *ofmt = output_format_context->oformat;

    if (!(ofmt->flags & AVFMT_NOFILE)) {
        if (output_file != NULL){
            LERROR("Attempted to open file that's already open");
            return false;
        }

        error = avio_open(&output_file, path.c_str(), AVIO_FLAG_WRITE);

        if (error < 0) {
            LERROR("Could not open output file '" + path + "'");
            LERROR("Error occurred: " + std::string(av_err2str(error)));
            return false;
        }

        if (output_format_context->pb != NULL){
            LERROR("Attempted to open buffer that's already open");
            return false;
        }

        error = avio_open_dyn_buf(&output_format_context->pb);

        if (error < 0) {
            LERROR("Could not open buffer");
            LERROR("Error occurred: " + std::string(av_err2str(error)));
            return false;
        }

    }

    //apply the mux options
    AVDictionary *opts = Util::get_dict_options(cfg.get_value("ffmpeg-mux-options"));

    if (segment_mode == SEGMENT_FMP4){
        //we need to make mp4s fragmented
        //empty_moov+separate_moof++dash++separate_moof+omit_tfhd_offset+default_base_moof"
        av_dict_set(&opts, "movflags", "+frag_custom+delay_moov+dash+skip_sidx+skip_trailer+empty_moov", 0);
    }

    if (segment_mode == SEGMENT_FMP4 && init_len >= 0){
        write_init();
    } else {
        error = avformat_write_header(output_format_context, &opts);
        av_dict_free(&opts);
        if (error < 0) {
            LERROR("Error occurred when opening output file");
            LERROR("Error occurred: " + std::string(av_err2str(error)));
            return false;
        }

        init_len = frag_stream();
    }
    av_dump_format(output_format_context, 0, path.c_str(), 1);

    file_opened = true;
    return true;

}

StreamWriter::~StreamWriter(){
    free_context();
}

long long StreamWriter::close(void){
    if (!file_opened){
        LWARN("Attempted to close a output stream that wasn't open");
        return 0;
    }
    //flush it...
    if (0 > av_interleaved_write_frame(output_format_context, NULL)){
        LERROR("Error flushing muxing output for camera " + cfg.get_value("camera-id"));
    }
    if (0 > av_write_frame(output_format_context, NULL)){
        LERROR("Error flushing muxing output for camera " + cfg.get_value("camera-id"));
    }

    if (segment_mode != SEGMENT_FMP4){
        av_write_trailer(output_format_context);
    }
    avio_flush(output_format_context->pb);
    write_buf(false);
    long long pos = avio_tell(output_file);
    avio_closep(&output_file);
    file_opened = false;
    return pos;
}

bool StreamWriter::write(const AVPacket &packet, const AVStream *in_stream){
    if (!file_opened){
        return false;
    }

    if (packet.stream_index >= static_cast<int>(stream_mapping.size()) ||
        stream_mapping[packet.stream_index] < 0) {
        return true;//we processed the stream we don't care about
    }

    AVStream *out_stream;
    PacketInterleavingBuf *pkt_buf = new PacketInterleavingBuf(packet);
    pkt_buf->video_keyframe = in_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && packet.flags & AV_PKT_FLAG_KEY;

    if (!pkt_buf->in || !pkt_buf->out){
        LERROR("Could not allocate new output packet for writing");
        delete pkt_buf;
        return false;
    }
    
    //log_packet(unwrap.get_format_context(), packet, "in: " + path);
    pkt_buf->out->stream_index = stream_mapping[pkt_buf->out->stream_index];
    out_stream = output_format_context->streams[pkt_buf->out->stream_index];

    /* copy packet */
    pkt_buf->out->pts = av_rescale_q_rnd(pkt_buf->out->pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
    pkt_buf->out->dts = av_rescale_q_rnd(pkt_buf->out->dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
    pkt_buf->out->duration = av_rescale_q(pkt_buf->out->duration, in_stream->time_base, out_stream->time_base);
    pkt_buf->out->pos = -1;

    /* This should actually be used only for exporting video
    //correct for the offset, it is intentional that we base the offset on DTS (always first), and subtract it from DTS and PTS to
    //preserve any difference between them
    if (stream_offset[stream_mapping[pkt_buf->out->stream_index]] < 0){
        stream_offset[stream_mapping[pkt_buf->out->stream_index]] = pkt_buf->out->dts;
    }
    pkt_buf->out->dts -= stream_offset[stream_mapping[pkt_buf->out->stream_index]];
    pkt_buf->out->pts -= stream_offset[stream_mapping[pkt_buf->out->stream_index]];
    */
    
    //guarentee that they have an increasing DTS
    if (pkt_buf->out->dts < last_dts[pkt_buf->out->stream_index]){
        LWARN("Shifting frame timestamp due to out of order issue in camera " + cfg.get_value("camera-id") +", old dts was: " + std::to_string(pkt_buf->out->dts));
        last_dts[pkt_buf->out->stream_index]++;
        long pts_delay = pkt_buf->out->pts - pkt_buf->out->dts;
        pkt_buf->out->dts = last_dts[pkt_buf->out->stream_index];
        pkt_buf->out->pts = pkt_buf->out->dts + pts_delay;
    } else if (pkt_buf->out->dts == last_dts[pkt_buf->out->stream_index]) {
        LWARN("Received duplicate frame from camera " + cfg.get_value("camera-id") +" at dts: " + std::to_string(pkt_buf->out->dts) + ". Dropping Frame");
        delete pkt_buf;
        return true;
    }

    last_dts[pkt_buf->out->stream_index] = pkt_buf->out->dts;
    //log_packet(output_format_context, out_pkt, "out: "+path);

    interleave(pkt_buf);
    return write_interleaved();
}

bool StreamWriter::write(PacketInterleavingBuf *pkt_buf){

    if (keyframe_cbk && pkt_buf->video_keyframe){
        keyframe_cbk(*(pkt_buf->in), *this);
    }
    LWARN("Writing: " + std::to_string(pkt_buf->dts0) + "(" + std::to_string(pkt_buf->out->dts) + ")");
    int ret = av_interleaved_write_frame(output_format_context, pkt_buf->out);
    if (ret < 0) {
        LERROR("Error muxing packet for camera " + cfg.get_value("camera-id"));
        return false;
    }

    delete pkt_buf;
    return true;
}

void StreamWriter::log_packet(const AVFormatContext *fmt_ctx, const AVPacket &pkt, const std::string &tag){
    AVRational *time_base = &fmt_ctx->streams[pkt.stream_index]->time_base;
    LINFO(tag + ": pts:" + std::string(av_ts2str(pkt.pts)) +
          " pts_time:"+std::string(av_ts2timestr(pkt.pts, time_base))+
          " dts:" + std::string(av_ts2str(pkt.dts)) +
          " dts_time:"+ std::string(av_ts2timestr(pkt.dts, time_base)) +
          " duration:" +std::string(av_ts2str(pkt.duration)) +
          " duration_time:" + std::string(av_ts2timestr(pkt.duration, time_base))+
          " stream_index:" + std::to_string(pkt.stream_index)
    );
    av_pkt_dump_log2(NULL, 30, &pkt, 0, fmt_ctx->streams[pkt.stream_index]);
}

long long StreamWriter::change_path(const std::string &new_path /* = "" */){
    if ((path == new_path || new_path.empty()) && segment_mode == SEGMENT_FMP4){
        return frag_stream();
    } else if (!new_path.empty()){
        long long pos = -1;
        if (file_opened){//we close and reopen only if already open, otherwise the caller must open() after adding streams
            pos = close();
            path = new_path;
            open_path();
        } else {
            path = new_path;

        }
        return pos;
    }
    return -1;
}

void StreamWriter::free_context(void){
    if (file_opened){
        close();
    }

    if (init_seg){
        init_len = -1;
        av_free(init_seg);
        init_seg = NULL;
    }

    //free our interleaving buffers
    for (auto ibuf : interleaving_buf){
        delete ibuf;
    }
    interleaving_buf.clear();

    avformat_free_context(output_format_context);

    output_format_context = NULL;
    stream_mapping.clear();

    for (auto &encoder :  encode_ctx){
        avcodec_free_context(&encoder.second);
    }

}

bool StreamWriter::alloc_context(void){
    if (output_format_context){
        return true;
    }
    avformat_alloc_output_context2(&output_format_context, NULL, NULL, path.c_str());
    if (!output_format_context) {
        LERROR("Could not create output context");
        int error = AVERROR_UNKNOWN;
        LERROR("Error occurred: " + std::string(av_err2str(error)));
        return false;
    }
    return true;
}

bool StreamWriter::add_stream(const AVStream *in_stream){
    AVStream *out_stream = init_stream(in_stream);
    if (out_stream == NULL){
        return false;
    }
    int error = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
    if (error < 0) {
        LERROR("Failed to copy codec parameters\n");
        LERROR("Error occurred: " + std::string(av_err2str(error)));
        return false;
    }

    return true;
}

bool StreamWriter::copy_streams(StreamUnwrap &unwrap){
    bool ret = false;
    for (unsigned int i = 0; i < unwrap.get_stream_count(); i++) {
        ret |= add_stream(unwrap.get_format_context()->streams[i]);
    }
    return ret;
}

bool StreamWriter::add_encoded_stream(const AVStream *in_stream, const AVCodecContext *dec_ctx){
    if (dec_ctx == NULL){
        return false;
    }

    AVStream *out_stream = init_stream(in_stream);
    if (out_stream == NULL){
        return false;
    }

    AVCodec *encoder = NULL;
    //Audio
    if (dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO){
        if (cfg.get_value("encode-format-audio") == "ac3"){
            encoder = avcodec_find_encoder(AV_CODEC_ID_AC3);
        }
        if (!encoder) {//default or above option(s) failed
            encoder = avcodec_find_encoder(AV_CODEC_ID_AAC);
        }
        if (encoder){
            encode_ctx[out_stream->index] = avcodec_alloc_context3(encoder);
            if (!encode_ctx[out_stream->index]){
                LERROR("Could not alloc audio encoding context");
                return false;
            }
            encode_ctx[out_stream->index]->sample_rate = dec_ctx->sample_rate;
            encode_ctx[out_stream->index]->channel_layout = dec_ctx->channel_layout;
            encode_ctx[out_stream->index]->channels = av_get_channel_layout_nb_channels(encode_ctx[out_stream->index]->channel_layout);
            encode_ctx[out_stream->index]->sample_fmt = encoder->sample_fmts[0];
            encode_ctx[out_stream->index]->time_base = (AVRational){1, encode_ctx[out_stream->index]->sample_rate};
        } else {
            LWARN("Could not find audio encoder");
            return false;
        }

    } else if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO){//video
        LWARN("Adding Video Stream!");
        if (cfg.get_value("encode-format-video") == "hevc"){
            encoder = avcodec_find_encoder(AV_CODEC_ID_HEVC);
        }
        if (!encoder) {//default or above option(s) failed
            if ((cfg.get_value("video-encode-method") == "auto" || cfg.get_value("video-encode-method") == "vaapi") &&
                gcff_util.have_vaapi(dec_ctx)){
                encoder = avcodec_find_encoder_by_name("h264_vaapi");
            }
            if (!encoder){
                encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
            }
        }
        if (encoder){
            encode_ctx[out_stream->index] = avcodec_alloc_context3(encoder);
            if (!encode_ctx[out_stream->index]){
                LERROR("Could not alloc video encoding context");
                return false;
            }
            encode_ctx[out_stream->index]->height = dec_ctx->height;
            encode_ctx[out_stream->index]->width = dec_ctx->width;
            encode_ctx[out_stream->index]->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;

            //take first pixel format
            encode_ctx[out_stream->index]->pix_fmt = dec_ctx->pix_fmt;
            encode_ctx[out_stream->index]->time_base = in_stream->time_base;//av_inv_q(dec_ctx->framerate);

            //set the fourcc
            if (encode_ctx[out_stream->index]->codec_id ==  AV_CODEC_ID_HEVC){
                encode_ctx[out_stream->index]->codec_tag = MKTAG('h', 'v', 'c', '1');//should be for hevc only?
            }

            //connect encoder
            if (!encode_ctx[out_stream->index]->hw_device_ctx &&
                (cfg.get_value("video-encode-method") == "auto" || cfg.get_value("video-encode-method") == "vaapi")){
                encode_ctx[out_stream->index]->hw_device_ctx = gcff_util.get_vaapi_ctx(encode_ctx[out_stream->index]);
                if (encode_ctx[out_stream->index]->hw_device_ctx){
                    encode_ctx[out_stream->index]->pix_fmt = AV_PIX_FMT_VAAPI;
                    if (dec_ctx->hw_frames_ctx){//this requires a decoded frame
                        encode_ctx[out_stream->index]->hw_frames_ctx = av_buffer_ref(dec_ctx->hw_frames_ctx);
                    } else {
                        LWARN("Didnt find frame ctx");
                    }
                }
            }
            //if it fails we get the SW encoder
        } else {
            LWARN("Could not find video encoder");
            return false;
        }
    } else {
        LINFO("Unknown stream type");
        return false;//not audio or video, can't encode it
    }

    
    if (output_format_context->oformat->flags & AVFMT_GLOBALHEADER){
        encode_ctx[out_stream->index]->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    gcff_util.lock();
    int ret = avcodec_open2(encode_ctx[out_stream->index], encoder, NULL);
    gcff_util.unlock();
    if (ret < 0) {
        LERROR("Cannot open encoder for stream " + std::to_string(out_stream->index));
        return false;
    }

    ret = avcodec_parameters_from_context(out_stream->codecpar, encode_ctx[out_stream->index]);
    if (ret < 0) {
        LERROR("Failed to copy encoder parameters to output stream " + std::to_string(out_stream->index));
        return false;
    }

    out_stream->time_base = encode_ctx[out_stream->index]->time_base;
    return true;
}

bool StreamWriter::write(const AVFrame *frame, const AVStream *in_stream){
    int ret = 0;
    AVPacket enc_pkt;
    av_init_packet(&enc_pkt);
    enc_pkt.data = NULL;
    enc_pkt.size = 0;
    ret = avcodec_send_frame(encode_ctx[stream_mapping[in_stream->index]], frame);
    if (ret < 0){
        LWARN("Error during encoding. Error code: " +  std::string(av_err2str(ret)));
        return false;
    }
    while (1) {
        ret = avcodec_receive_packet(encode_ctx[stream_mapping[in_stream->index]], &enc_pkt);
        if (ret){
            if (ret == AVERROR(EAGAIN)){
                break;
            }
            LWARN("Error Receiving Packet");
            break;
        }

        enc_pkt.stream_index = in_stream->index;//revert stream index because write will adjust this
        bool write_ret = write(enc_pkt, in_stream);
        av_packet_unref(&enc_pkt);
        if (!write_ret){
            return false;
        }
    }
    return true;
}

AVStream *StreamWriter::init_stream(const AVStream *in_stream){
    if (!output_format_context){
        if (!alloc_context()){
            return NULL;
        }
    }

    if (in_stream == NULL){
        return NULL;
    }

    AVStream *out_stream = NULL;
    AVCodecParameters *in_codecpar = in_stream->codecpar;

    if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
        in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
        stream_mapping[in_stream->index] = -1;
        return NULL;
    }

    stream_mapping[in_stream->index] = output_format_context->nb_streams;

    //set the offset to -1, indicating unknown
    stream_offset.push_back(-1);
    last_dts.push_back(LONG_MIN);

    out_stream = avformat_new_stream(output_format_context, NULL);
    if (!out_stream) {
        LERROR("Failed allocating output stream");
        LERROR("Error occurred: " + std::string(av_err2str(AVERROR_UNKNOWN)));
        stream_mapping[in_stream->index] = -1;
        return NULL;
    }

    return out_stream;
}

bool StreamWriter::is_fragmented(void){
    return segment_mode == SEGMENT_FMP4;
}

long long StreamWriter::frag_stream(void){
    if (!is_fragmented()){
        return -1;
    }

    //flush the buffers
    av_interleaved_write_frame(output_format_context, NULL);
    av_write_frame(output_format_context, NULL);
    avio_flush(output_format_context->pb);
    long long written = write_buf(true);
    if (written < 0){
        return -1;
    }
    long long pos = avio_tell(output_file);
    return pos;

}

long long StreamWriter::get_init_len(void){
    return init_len;
}

void StreamWriter::set_keyframe_callback(std::function<void(const AVPacket &pkt, StreamWriter &out)> cbk){
    keyframe_cbk = cbk;
}

long long StreamWriter::write_buf(bool reopen){
    if (output_file == NULL || output_format_context == NULL || output_format_context->pb == NULL){
        LWARN("Attempted to write a buffer to a file when both don't exist");
        return -1;
    }

    uint8_t *buf;
    long long len;
    len = avio_close_dyn_buf(output_format_context->pb, &buf);
    avio_write(output_file, buf, len);
    if (init_seg == NULL){
        init_seg = buf;
        init_len = len;
    } else {
        av_free(buf);
    }
    if (reopen){
        int error = avio_open_dyn_buf(&output_format_context->pb);

        if (error < 0) {
            LERROR("Could not open buffer");
            LERROR("Error occurred: " + std::string(av_err2str(error)));
            return -2;
        }
    } else {
        output_format_context->pb = NULL;
    }
    return len;
}

long long StreamWriter::write_init(void){
    if (init_seg == NULL || init_len < 0){
        return -1;
    }
    if (init_len == 0){
        return 0;
    }
    avio_write(output_file, init_seg, init_len);
    return init_len;
}

void StreamWriter::interleave(PacketInterleavingBuf *buf){
    if (buf->out->stream_index == 0){
        buf->dts0 = buf->out->dts;
    } else {
        //convert to the stream 0 timebase
        buf->dts0 = av_rescale_q_rnd(buf->out->dts,
                                     output_format_context->streams[buf->out->stream_index]->time_base,
                                     output_format_context->streams[0]->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
    }
    interleaved_dts[buf->out->stream_index] = buf->dts0;
    //find the first buf where dts0 is > than ours, and insert just before that
    for (auto ibuf = interleaving_buf.begin(); ibuf != interleaving_buf.end(); ibuf++){
        if ((*ibuf)->dts0 > buf->dts0){
            interleaving_buf.insert(ibuf, buf);
            return;
        }
    }
    interleaving_buf.push_back(buf);
}

bool StreamWriter::write_interleaved(void){
    unsigned int stream_count = output_format_context->nb_streams;

    //do not continue if we don't all streams
    if (interleaved_dts.size() != stream_count){
        if (interleaving_buf.size() > 100){
            LWARN("Over 100 packets buffered and all streams not seen yet");
            return false;
        } else {
            LDEBUG("Waiting for packet, interleaving_buf has " + std::to_string(interleaving_buf.size()));
            return true;
        }
    }

    //find the earilest DTS across all streams
    long long last_dts = LLONG_MAX;
    for (const auto dts : interleaved_dts){
        if (dts.second < last_dts){
            last_dts = dts.second;
        }
    }

    bool ret = true;
    for (auto ibuf = interleaving_buf.begin(); ibuf != interleaving_buf.end();){
        if ((*ibuf)->dts0 <= last_dts){
            ret &= write(*ibuf);
            ibuf = interleaving_buf.erase(ibuf);
        } else {
            break;
        }
    }

    return ret;
}

PacketInterleavingBuf::PacketInterleavingBuf(const AVPacket &pkt){
    in = av_packet_clone(&pkt);
    out = av_packet_clone(&pkt);
    video_keyframe = false;
}

PacketInterleavingBuf::~PacketInterleavingBuf(){
    av_packet_free(&in);
    av_packet_free(&out);
}
