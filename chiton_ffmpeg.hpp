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
 *   Copyright 2020-2023 Ed Martin <edman007@edman007.com>
 *
 **************************************************************************
 */

#include "chiton_config.hpp"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/timestamp.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <libavutil/hwcontext.h>
};
#include <mutex>
#include <vector>
#include <map>
#ifdef HAVE_VDPAU
#include <vdpau/vdpau.h>
#include <libavutil/hwcontext_vdpau.h>
#endif

#ifdef HAVE_VAAPI
#include <libavutil/hwcontext_vaapi.h>
#endif

#ifdef HAVE_OPENCL
#include <libavutil/hwcontext_opencl.h>
#endif

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


#ifdef av_fourcc2str
#undef av_fourcc2str
av_always_inline char* av_fourcc2str(uint32_t fourcc){
    thread_local static char str[AV_FOURCC_MAX_STRING_SIZE];
    memset(str, 0, sizeof(str));
    return av_fourcc_make_string(str, fourcc);
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
    AVBufferRef *get_vaapi_ctx(AVCodecID codec_id, int codec_profile, int width, int height);
    AVBufferRef *get_vdpau_ctx(AVCodecID codec_id, int codec_profile, int width, int height);
    AVBufferRef *get_opencl_ctx(AVCodecID codec_id, int codec_profile, int width, int height);

    bool have_vaapi(AVCodecID codec_id, int codec_profile, int width, int height);//returns true if VAAPI should work
    bool have_vdpau(AVCodecID codec_id, int codec_profile, int width, int height);//returns true if VDPAU should work
    bool have_opencl(AVCodecID codec_id, int codec_profile, int width, int height);//returns true if OPENCL should work
    bool have_v4l2(AVCodecID codec_id, int codec_profile, int width, int height);//returns true if v4l2 should work
    bool sw_format_is_hw_compatable(const enum AVPixelFormat pix_fmt);//return true if the format is HW compatable
    std::string get_sw_hw_format_list(Config &cfg, const AVFrame *frame, AVCodecID codec_id, int codec_profile);//return the suggested list of formats for use with later HW functions
private:
    void load_vaapi(void);//init global vaapi context
    void free_vaapi(void);//free the vaapi context
    void load_vdpau(void);//init global vdpau context
    void free_vdpau(void);//free the vdpau context
    void load_opencl(void);//init global opencl context
    void free_opencl(void);//free the opencl context

#ifdef HAVE_VDPAU
    int  get_vdpau_profile(const AVCodecID codec_id, const int codec_profile, VdpDecoderProfile *profile);//get the VDPAU Profile
#endif
#ifdef HAVE_V4L2
    bool have_v4l2_dev(int devfd, AVCodecID codec_id, int codec_profile, int width, int height);//check v4l2 capabilities of device
    bool vioctl(int fh, int request, void *arg);//run ioctl through v4l2, return false on error
    bool is_v4l2_hw_codec(const AVCodecID av_codec, const uint32_t v4l2_pix_fmt);
#endif
#ifdef HAVE_VAAPI
    AVPixelFormat get_pix_fmt_from_va(const VAImageFormat &fmt);
    VAProfile get_va_profile(AVVAAPIDeviceContext* hwctx, AVCodecID codec_id, int codec_profile);//return the VAProfile for the codec
#endif
    AVBufferRef *vaapi_ctx = NULL;
    bool vaapi_failed = false;//if we failed to initilize vaapi
    AVBufferRef *vdpau_ctx = NULL;
    bool vdpau_failed = false;//if we failed to initilize vdpau
    AVBufferRef *opencl_ctx = NULL;
    bool opencl_failed = false;//if we failed to initilize opencl
    std::mutex codec_lock;
};

enum AVPixelFormat get_vaapi_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts);//global VAAPI format selector
enum AVPixelFormat get_vdpau_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts);//global VDPAU format selector
enum AVPixelFormat get_sw_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts);//global SW format selector that prefers VAAPI compatible formats

//helper functions to cast FFMPeg HW Context to the native context
#ifdef HAVE_VAAPI
AVVAAPIDeviceContext *get_vaapi_ctx_from_device(AVBufferRef *buf);//get VAAPI context from HW Device ctx
AVVAAPIDeviceContext *get_vaapi_ctx_from_frames(AVBufferRef *buf);//get VAAPI context from HW Frames Ctx
#endif
#ifdef HAVE_OPENCL
AVOpenCLDeviceContext *get_opencl_ctx_from_device(AVBufferRef *buf);//get OpenCL context from HW Device ctx
AVOpenCLDeviceContext *get_opencl_ctx_from_frames(AVBufferRef *buf);//get OpenCL context from HW Frames Ctx
#endif
extern CFFUtil gcff_util;//global FFmpeg lib mangement class

//for passing image coordinates
struct rect { int x, y, w, h; };
#endif
