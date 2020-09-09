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
    StreamWriter(Config &cfg, std::string path, StreamUnwrap &unwrap);
    ~StreamWriter();
    
    bool open();//open the file for writing, returns true on success
    void close(void);
    bool write(const AVPacket &pkt);//write the packet to the file
    void change_path(std::string &new_path);
private:
    Config &cfg;
    Config *cfg1;
    Config *cfg2;
    std::string path;
    StreamUnwrap &unwrap;
    
    AVFormatContext *output_format_context = NULL;
    int stream_mapping_size = 0;
    int *stream_mapping = NULL;

    //these offsets are used to shift the time when receiving something
    std::vector<long> stream_offset;
    std::vector<long> last_dts;//used to fix non-increasing DTS

    //used to track if we were successful in getting a file opened (and skip writing anything if not successful)
    //true means the file is opened
    bool file_opened;
    
    void log_packet(const AVFormatContext *fmt_ctx, const AVPacket &pkt, const std::string &tag);
};
#endif
