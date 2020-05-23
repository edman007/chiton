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
    StreamWriter(Config cfg, std::string path, StreamUnwrap &unwrap) : cfg(cfg), path(path), unwrap(unwrap) {};
    ~StreamWriter();
    
    bool  open();//open the file for writing, returns true on success
    void close(void);
    bool write(const AVPacket &pkt);//write the packet to the file
private:
    Config &cfg;
    std::string path;
    StreamUnwrap &unwrap;
    
    AVFormatContext *output_format_context = NULL;
    int stream_mapping_size = 0;
    int *stream_mapping = NULL;
    
};
#endif
