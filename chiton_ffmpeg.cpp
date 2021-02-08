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

std::mutex global_codec_lock;
thread_local std::string last_line;//contains the continuation of any line that doesn't have a \n
AVBufferRef *global_vaapi_ctx = NULL;

void ffmpeg_log_callback(void * avcl, int level, const char * fmt, va_list vl){
    //compute the level first...
    enum LOG_LEVEL chiton_level;
    if (level <= AV_LOG_FATAL){
        chiton_level = CH_LOG_FATAL;
    } else if (level <= AV_LOG_ERROR){
        chiton_level = CH_LOG_WARN;
    } else if (level <= AV_LOG_WARNING){
        chiton_level = CH_LOG_INFO;
    } else if (level <= AV_LOG_INFO){
        chiton_level = CH_LOG_INFO;
    } else if (level <= AV_LOG_VERBOSE){
        chiton_level = CH_LOG_DEBUG;
    /* Uncomment to see these messages, they do really cause too much output */
    /*
    } else if (level <= AV_LOG_DEBUG){
        chiton_level = CH_LOG_DEBUG;
    } else if (level <= AV_LOG_TRACE){
        chiton_level = CH_LOG_DEBUG;
    */
    } else {
        return;//higher level stuff is ignored!
    }

    //format the message
    char buf[1024];
    int len = std::vsnprintf(buf, sizeof(buf), fmt, vl);
    if (len >= 1){
        //strip out the \n...
        if (buf[len - 1] == '\n'){
            buf[len - 1] = '\0';
        } else {
            last_line += buf;
            return;
        }
        //and write it
        Util::log_msg(chiton_level, last_line + buf);
        last_line.clear();
    }
}

void load_ffmpeg(void){
#ifdef DEBUG
    av_log_set_level(AV_LOG_DEBUG);
#else
    av_log_set_level(AV_LOG_WARNING);
#endif
    av_log_set_callback(ffmpeg_log_callback);

    //init vaapi
    global_vaapi_ctx = NULL;
}

void load_vaapi(void){
    if (global_vaapi_ctx){
        return;//don't double alloc it
    }

    //FIXME: these NULLs should be user options
    int ret = av_hwdevice_ctx_create(&global_vaapi_ctx, AV_HWDEVICE_TYPE_VAAPI, NULL, NULL, 0);
    if (ret < 0) {
        global_vaapi_ctx = NULL;
        return;
    }
}

void free_vaapi(void){
    av_buffer_unref(&global_vaapi_ctx);
}

//ripped from vaapi_transcode.c ffmpeg demo
enum AVPixelFormat get_vaapi_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts) {
    const enum AVPixelFormat *p;
    for (p = pix_fmts; *p != AV_PIX_FMT_NONE; p++){
        if (*p == AV_PIX_FMT_VAAPI){
            return *p;
        }
    }
    return AV_PIX_FMT_NONE;//should this be pix_fmts?
}
