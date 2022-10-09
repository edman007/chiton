#include "cfmp4.hpp"
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
 *   Copyright 2021 Ed Martin <edman007@edman007.com>
 *
 **************************************************************************
 */
#include "io/cfmp4.hpp"
#include "util.hpp"
#include <assert.h>

//I hope this is implied...but maybe not and this will actually help somebody someday
static_assert(sizeof(std::basic_istream<char>::char_type) == sizeof(uint8_t), "libav* uint8_t does NOT match libc++ charT, evaluate the casts in this file");

const int cfmp4_buf_size = 8192;
CFMP4::CFMP4(const std::string &path, int initlen, int start_byte, int end_byte) : initlen(initlen), start_byte(start_byte), end_byte(end_byte) {
    LWARN("Will open " + path);
    if (end_byte > start_byte){
        //open the file
        ifs.open(path, std::ios::binary|std::ios::in);
        if (!ifs.good()){
            LWARN("Failed to open " + path);
        }
    }

    pb = NULL;
    if (ifs.is_open()){
        buf = static_cast<unsigned char*>(av_malloc(cfmp4_buf_size));
        pb = avio_alloc_context(buf, cfmp4_buf_size, 0, this, read_packet, 0, seek);
        LWARN("Opened!");
    } else {
        //failed...just set them to zero
        buf = NULL;
        pb = NULL;
    }
}

CFMP4::~CFMP4(){
    avio_context_free(&pb);

}

AVIOContext* CFMP4::get_pb(void){
    LWARN("Got PB ");
    return pb;
}

int CFMP4::read_packet(void *opaque, uint8_t *buf, int buf_size){
    CFMP4* stream = static_cast<CFMP4*>(opaque);
    return stream->read(buf, buf_size);
}

int CFMP4::read(uint8_t *buf, int buf_size){
    LINFO("Reading from custom IO");
    if (ifs.eof()){
        LDEBUG("at EOF");
        return AVERROR_EOF;
    }
    if (!ifs.good()){
        return AVERROR(EINVAL);
    }

    auto pos = ifs.tellg();
    LDEBUG("pos: " + std::to_string(pos) + " end:" + std::to_string(end_byte));
    if (pos < 0){
        return AVERROR(EINVAL);//error?
    }
    if (pos >= end_byte){
        LDEBUG("Hit EOF");
        //EOF
        return AVERROR_EOF;
    }

    //read up to initlen
    if (pos < initlen){
        int len = initlen - pos;
        if (len > buf_size){
            len = buf_size;
        }
        ifs.read(reinterpret_cast<std::basic_istream<char>::char_type*>(buf), len);
        return ifs.gcount();//we don't bother skipping past the init segment
    }

    if (pos < start_byte){
        //seek to the start byte
        ifs.seekg(start_byte);
        if (!ifs.good()){
            LWARN("Failed to seek to segment start");
            return AVERROR(EINVAL);
        }
        pos = start_byte;
    }

    //and read up to the end
    int len = end_byte - pos;
    if (len > buf_size){
        len = buf_size;
    }
    ifs.read(reinterpret_cast<std::basic_istream<char>::char_type*>(buf), len);
    return ifs.gcount();
}

int64_t CFMP4::seek(void *opaque, int64_t offset, int whence){
    CFMP4* stream = static_cast<CFMP4*>(opaque);
    return stream->seek(offset, whence);
}

int64_t CFMP4::seek(int64_t offset, int whence){
    if (ifs.fail()){
        return AVERROR(EINVAL);
    }

    //should we return an error if the final position is at EOF or our virtual EOF?
    if (whence == SEEK_CUR){
        auto pos = ifs.tellg();
        int target = pos + offset;
        if (pos < start_byte && target >= initlen){
            target -= start_byte - initlen;
        }
        ifs.seekg(target);
        pos = ifs.tellg();
        if (pos > initlen){
            pos -= start_byte - initlen;
        }
        return pos;
    } else if (whence == SEEK_SET){
        if (offset >= initlen){
            offset += start_byte - initlen;
        }
        ifs.seekg(offset);
        auto pos = ifs.tellg();
        if (pos > initlen){
            pos -= start_byte - initlen;
        }
        return pos;
    } else if (whence == SEEK_END){
        auto target = end_byte + offset;
        if (target > end_byte){
            ifs.seekg(end_byte);
            return AVERROR_EOF;
        }
        if (target >= start_byte){
            ifs.seekg(target);
            auto start_pos = (start_byte - initlen);
            return ifs.tellg() - static_cast<std::streampos>(start_pos);
        }
        target -= start_byte - initlen;
        if (target < 0){
            return AVERROR(EINVAL);//error, before the file
        }
        ifs.seekg(target);
        return ifs.tellg();

    } else {
        return AVERROR(EINVAL);
    }
}

std::string CFMP4::get_url(void){
    return "cfmp4://" + std::to_string(start_byte);//this isn't really a full URL...currently dead code so meh
}
