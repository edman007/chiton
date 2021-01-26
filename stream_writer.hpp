#ifndef __STREAM_WRITER_HPP__
#define __STREAM_WRITER_HPP__
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
#include "stream_unwrap.hpp"

class StreamWriter {
public:
    StreamWriter(Config &cfg, std::string path);
    StreamWriter(Config &cfg);
    ~StreamWriter();
    
    bool open(void);//open the file for writing, returns true on success
    void close(void);
    bool write(const AVPacket &pkt, const AVStream *in_stream);//write the packet to the file
    bool write(const AVFrame *frame, const AVStream *in_stream);//write the frame to the file (using encoding)
    void change_path(std::string &new_path);
    bool add_stream(const AVStream *in_stream);//add in_stream to output (copy)
    bool add_encoded_stream(const AVStream *in_stream, const AVCodecContext *dec_ctx);//add in_stream to output, using encoding
    bool copy_streams(StreamUnwrap &unwrap);//copy all streams from unwrap to output
private:
    Config &cfg;
    Config *cfg1;
    Config *cfg2;
    std::string path;
    //StreamUnwrap &unwrap;
    
    AVFormatContext *output_format_context = NULL;
    std::map<int,int> stream_mapping;
    std::map<int, AVCodecContext*> encode_ctx;
    //these offsets are used to shift the time when receiving something
    std::vector<long> stream_offset;
    std::vector<long> last_dts;//used to fix non-increasing DTS

    //used to track if we were successful in getting a file opened (and skip writing anything if not successful)
    //true means the file is opened
    bool file_opened;
    
    void log_packet(const AVFormatContext *fmt_ctx, const AVPacket &pkt, const std::string &tag);

    void free_context(void);//free the context associated with a file
    bool alloc_context(void);
    AVStream *init_stream(const AVStream *in_stream);//Initilizes output stream
};
#endif
