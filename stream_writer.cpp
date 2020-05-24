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
#include "stream_writer.hpp"
#include "util.hpp"
#include "chiton_ffmpeg.hpp"

bool StreamWriter::open(void){
    //need to free input_ctx
    /*
    AVCodecContext *input_ctx = unwrap.alloc_decode_context(0);
    if (!input_ctx){
        return false;
    }
    */
    
    int error;

    avformat_alloc_output_context2(&output_format_context, NULL, NULL, path.c_str());
    if (!output_format_context) {
        LERROR("Could not create output context");
        error = AVERROR_UNKNOWN;
        LERROR("Error occurred: " + std::string(av_err2str(error)));
        return false;
    }

    stream_mapping_size = unwrap.get_stream_count();
    stream_mapping = NULL;
    stream_mapping = (int*)av_mallocz_array(stream_mapping_size, sizeof(*stream_mapping));
    if (!stream_mapping) {
        error = AVERROR(ENOMEM);
        LERROR("Error occurred: " + std::string(av_err2str(error)));
        return false;
    }

    
    AVOutputFormat *ofmt = output_format_context->oformat;
    int stream_index = 0;
    for (unsigned int i = 0; i < unwrap.get_stream_count(); i++) {
        AVStream *out_stream;
        AVStream *in_stream = unwrap.get_format_context()->streams[i];
        AVCodecParameters *in_codecpar = in_stream->codecpar;

        if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
            stream_mapping[i] = -1;
            continue;
        }

        stream_mapping[i] = stream_index++;

        out_stream = avformat_new_stream(output_format_context, NULL);
        if (!out_stream) {
            LERROR("Failed allocating output stream");
            error = AVERROR_UNKNOWN;
            LERROR("Error occurred: " + std::string(av_err2str(error)));
            return false;
        }

        error = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
        if (error < 0) {
            LERROR("Failed to copy codec parameters\n");
            LERROR("Error occurred: " + std::string(av_err2str(error)));
            return false;
        }
        out_stream->codecpar->codec_tag = 0;
    }
    av_dump_format(output_format_context, 0, path.c_str(), 1);
    
    if (!(ofmt->flags & AVFMT_NOFILE)) {
        error = avio_open(&output_format_context->pb, path.c_str(), AVIO_FLAG_WRITE);
        if (error < 0) {
            LERROR("Could not open output file '" + path + "'");
            LERROR("Error occurred: " + std::string(av_err2str(error)));
            return false;
        }
    }

    error = avformat_write_header(output_format_context, NULL);
    if (error < 0) {
        LERROR("Error occurred when opening output file");
        LERROR("Error occurred: " + std::string(av_err2str(error)));
        return false;
    }
    return true;

}


StreamWriter::~StreamWriter(){
    /* close output */
    if (output_format_context && !(output_format_context->flags & AVFMT_NOFILE))
        avio_closep(&output_format_context->pb);
    avformat_free_context(output_format_context);

    av_freep(&stream_mapping);
}

void StreamWriter::close(void){
    //flush it...
    if (0 > av_interleaved_write_frame(output_format_context, NULL)){
        LERROR("Error flushing muxing output");
    }

    av_write_trailer(output_format_context);
}

bool StreamWriter::write(const AVPacket &packet){
    AVStream *in_stream, *out_stream;
    AVPacket out_pkt;
    if (av_packet_ref(&out_pkt, &packet)){
        LERROR("Could not allocate new output packet for writing");
        return false;
    }
    
    in_stream  = unwrap.get_format_context()->streams[out_pkt.stream_index];
    if (out_pkt.stream_index >= stream_mapping_size ||
        stream_mapping[out_pkt.stream_index] < 0) {
        av_packet_unref(&out_pkt);
        return true;//we processed the stream we don't care about
    }
    log_packet(unwrap.get_format_context(), packet, "in-" + path);
    out_pkt.stream_index = stream_mapping[out_pkt.stream_index];
    out_stream = output_format_context->streams[out_pkt.stream_index];

    /* copy packet */
    out_pkt.pts = av_rescale_q_rnd(out_pkt.pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
    out_pkt.dts = av_rescale_q_rnd(out_pkt.dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
    out_pkt.duration = av_rescale_q(out_pkt.duration, in_stream->time_base, out_stream->time_base);
    out_pkt.pos = -1;
    log_packet(output_format_context, out_pkt, "out-"+path);

    int ret = av_interleaved_write_frame(output_format_context, &out_pkt);
    if (ret < 0) {
        LERROR("Error muxing packet");
        return false;
    }

    av_packet_unref(&out_pkt);
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
}
