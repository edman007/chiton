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

class StreamUnwrap {
    /*
     * Handles the Network half of the video
     *
     */
public:
    StreamUnwrap(Config& cfg);
    ~StreamUnwrap();
    
    bool connect(void);//returns true on success
    AVCodecContext* get_codec_context(void);
    AVFormatContext* get_format_context(void);
    unsigned int get_stream_count(void);
    AVCodecContext* alloc_decode_context(unsigned int stream);//alloc and return the codec context for the stream, caller must free it
    bool get_next_frame(AVPacket &packet);//writes the next frame out to packet, returns true on success, false on error (end of file)
    void unref_frame(AVPacket &packet);//free resources from frame
private:
    const std::string url;//the URL of the camera we are connecting to
    Config& cfg;
    AVDictionary *get_options(void);
    void dump_options(AVDictionary* dict);

    AVFormatContext *input_format_context;
    AVPacket pkt;
};

#endif
