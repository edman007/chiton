#ifndef __IO_CFMP4_HPP__
#define __IO_CFMP4_HPP__
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

#include "io/io_wrapper.hpp"
#include <fstream>

/*
 * Supports reading of Chiton-generated fMP4 segments
 */

class CFMP4 : public IOWrapper {
public:
    CFMP4(const std::string &path, int initlen, int start_byte, int end_byte);
    ~CFMP4() override;

    AVIOContext* get_pb(void) override;//return the pb handle refering to this
    std::string get_url(void) override;//return a URL for this stream
private:
    AVIOContext* pb;//the pb we own, automatically freed upon destruction of this object
    unsigned char* buf;//the buffer used for reading

    //segment metadata
    const int initlen;//length of the init segment, prepended to the segment
    const int start_byte;//start of the segment
    const int end_byte;//end of the segment

    std::ifstream ifs;

    //API interface call
    static int read_packet(void *opaque, uint8_t *buf, int buf_size);
    static int64_t seek(void *opaque, int64_t offset, int whence);

    //actual calls to handle API
    int read(uint8_t *buf, int buf_size);
    int64_t seek(int64_t offset, int whence);
};

#endif
