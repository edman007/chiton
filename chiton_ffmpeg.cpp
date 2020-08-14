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
#include "chiton_ffmpeg.hpp"
#include "util.hpp"
#include <cstdio>

void ffmpeg_log_callback(void * avcl, int level, const char * fmt, va_list vl){
    //compute the level first...
    enum LOG_LEVEL chiton_level;
    if (level <= AV_LOG_FATAL){
        chiton_level = LOG_FATAL;
    } else if (level <= AV_LOG_ERROR){
        chiton_level = LOG_ERROR;
    } else if (level <= AV_LOG_WARNING){
        chiton_level = LOG_WARN;
    } else if (level <= AV_LOG_INFO){
        chiton_level = LOG_INFO;
    //}else if (level <= AV_LOG_VERBOSE){//we do not have a "verbose"
    } else {
        chiton_level = LOG_DEBUG;
    }

    //format the message
    char buf[1024];
    int len = std::vsnprintf(buf, sizeof(buf), fmt, vl);
    if (len > 1){
        //strip out the \n...
        if (buf[len - 1] == '\n'){
            buf[len - 1] = '\0';
        }
        //and write it
        Util::log_msg(chiton_level, buf);

    }
}

void load_ffmpeg(void){
#ifdef DEBUG
    av_log_set_level(AV_LOG_DEBUG);
#else
    av_log_set_level(AV_LOG_INFO);
#endif
    av_log_set_callback(ffmpeg_log_callback);
}
