#ifndef __STREAM_UNWRAP_HPP__
#define __STREAM_UNWRAP_HPP__
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

#include "chiton_config.hpp"
#include "chiton_ffmpeg.hpp"
#include <list>
#include <vector>
#include <map>

class StreamUnwrap {
    /*
     * Handles the Network half of the video
     *
     */
public:
    StreamUnwrap(Config& cfg);
    ~StreamUnwrap();
    
    bool connect(void);//returns true on success
    bool close(void);//close the connection

    AVCodecContext* get_codec_context(void);
    AVFormatContext* get_format_context(void);
    unsigned int get_stream_count(void);
    bool get_next_frame(AVPacket &packet);//writes the next frame out to packet, returns true on success, false on error (end of file)
    bool decode_packet(AVPacket &packet);//reads packet and decodes it
    bool get_decoded_frame(int stream, AVFrame *frame);//gets the next decoded frame
    void unref_frame(AVPacket &packet);//free resources from frame
    void timestamp(const AVPacket &packet, struct timeval &time);//write the actual timestamp of packet to time
    bool is_audio(const AVPacket &packet);//return true if packet is audio packet
    bool is_video(const AVPacket &packet);//return true if packet is video packet
    AVStream *get_stream(const AVPacket &packet);//return a pointer to the stream that packet is part of
    AVStream *get_stream(const int id);//return a pointer to the stream given the stream index
    const struct timeval& get_start_time(void);
    
private:
    const std::string url;//the URL of the camera we are connecting to
    Config& cfg;
    AVDictionary *get_options(void);
    void dump_options(AVDictionary* dict);
    bool charge_reorder_queue(void);//loads frames until the reorder queue is full
    void sort_reorder_queue(void);//ensures the last frame is in the correct position in the queue, sorting it if required
    bool read_frame(void);//read the next frame (internally used)
    bool alloc_decode_context(unsigned int stream);//alloc the codec context, returns true if the context exists or was allocated
    
    AVFormatContext *input_format_context;
    std::map<int,AVCodecContext*> decode_ctx;//decode context for each stream

    unsigned int reorder_len;
    std::list<AVPacket> reorder_queue;

    struct timeval connect_time;
    int max_sync_offset;
};

#endif
