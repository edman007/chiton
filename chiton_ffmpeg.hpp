#ifndef __CHITON_FFMPEG_HPP__
#define __CHITON_FFMPEG_HPP__
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
 *   Copyright 2020-2021 Ed Martin <edman007@edman007.com>
 *
 **************************************************************************
 */

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/timestamp.h>
#include <libavutil/avutil.h>
};
#include <mutex>

//fix FFMPEG and c++1x issues
#ifdef av_err2str
#undef av_err2str
av_always_inline char* av_err2str(int errnum){
    thread_local static char str[AV_ERROR_MAX_STRING_SIZE];
    memset(str, 0, sizeof(str));
    return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
}
#endif

#ifdef av_ts2timestr
#undef av_ts2timestr
av_always_inline char* av_ts2timestr(int64_t ts, AVRational * tb){
    thread_local static char str[AV_TS_MAX_STRING_SIZE];
    memset(str, 0, sizeof(str));
    return av_ts_make_time_string(str, ts, tb);
}
#endif

#ifdef av_ts2str
#undef av_ts2str
av_always_inline char* av_ts2str(int64_t ts){
    thread_local static char str[AV_TS_MAX_STRING_SIZE];
    memset(str, 0, sizeof(str));
    return av_ts_make_string(str, ts);
}
#endif


//make AVRounding valid in bitwise operations
inline AVRounding operator|(AVRounding a, AVRounding b)
{
    return static_cast<AVRounding>(static_cast<int>(a) | static_cast<int>(b));
}

/*
 * Utility/Mangement class for libav* and ffmpeg things
 */
class CFFUtil {
public:
    CFFUtil(void);
    ~CFFUtil(void);
    void load_ffmpeg(void);//init ffmpeg
    void free_hw(void);//free/close HW encoder/decoders
    void lock(void);//global ffmpeg lock/unlock
    void unlock(void);

    //return a ref or null to the context iff it can handle wxh
    AVBufferRef *get_vaapi_ctx(const AVCodecContext* avctx);
    AVBufferRef *get_vdpau_ctx(const AVCodecContext* avctx);

    bool have_vaapi(const AVCodecContext* avctx);//returns true if VAAPI should work
    bool have_vdpau(const AVCodecContext* avctx);//returns true if VDPAU should work
private:
    void load_vaapi(void);//init global vaapi context
    void free_vaapi(void);//free the vaapi context
    void load_vdpau(void);//init global vdpau context
    void free_vdpau(void);//free the vdpau context


    AVBufferRef *vaapi_ctx = NULL;
    bool vaapi_failed = false;//if we failed to initilize vaapi
    AVBufferRef *vdpau_ctx = NULL;
    bool vdpau_failed = false;//if we failed to initilize vdpau
    std::mutex codec_lock;

};

enum AVPixelFormat get_vaapi_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts);//global VAAPI format selector
enum AVPixelFormat get_vdpau_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts);//global VDPAU format selector
extern CFFUtil gcff_util;//global FFmpeg lib mangement class

//for passing image coordinates
struct rect { int x, y, w, h; };
#endif
