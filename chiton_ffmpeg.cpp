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
 *   Portions of this file have been borrowed and modified from FFmpeg,
 *    these portions have been origionally licensed under GPL 2.1
 *
 **************************************************************************
 */

#include "util.hpp"
#include <cstdio>
#include <sstream>
#include <vector>
#ifdef HAVE_VAAPI
#include <va/va.h>
#endif


#ifdef HAVE_OPENCL
#ifdef HAVE_OPENCL_CL_H
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif
#endif

#ifdef HAVE_OPENCV
#include <opencv2/core/ocl.hpp>
#endif

#ifdef HAVE_V4L2
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <libv4l2.h>

#endif

thread_local std::string last_line;//contains the continuation of any line that doesn't have a \n

CFFUtil gcff_util;//global instance of our util class

// For converting from FFMPEG codecs to VAAPI codecs (see ffmpeg_vaapi.c from FFmpeg)
static const struct {
     enum AVCodecID codec_id;
     int codec_profile;
     VAProfile va_profile;
 } vaapi_profile_map[] = {
#define MAP(c, p, v) { AV_CODEC_ID_ ## c, FF_PROFILE_ ## p, VAProfile ## v }
    MAP(H264,        H264_MAIN,       H264Main    ),
    MAP(H264,        H264_HIGH,       H264High    ),
    MAP(H264,        H264_CONSTRAINED_BASELINE, H264ConstrainedBaseline),
#if !VA_CHECK_VERSION(1, 0, 0)
    MAP(H264,        H264_BASELINE,   H264Baseline),
#endif
#if VA_CHECK_VERSION(0, 37, 0)
    MAP(HEVC,        HEVC_MAIN,       HEVCMain    ),
#endif
    /* These codecs might be used for decoding */
    MAP(WMV3,        VC1_SIMPLE,      VC1Simple   ),
    MAP(WMV3,        VC1_MAIN,        VC1Main     ),
    MAP(WMV3,        VC1_COMPLEX,     VC1Advanced ),
    MAP(WMV3,        VC1_ADVANCED,    VC1Advanced ),
    MAP(MPEG2VIDEO,  MPEG2_SIMPLE,    MPEG2Simple ),
    MAP(MPEG2VIDEO,  MPEG2_MAIN,      MPEG2Main   ),
    MAP(VC1,         VC1_SIMPLE,      VC1Simple   ),
    MAP(VC1,         VC1_MAIN,        VC1Main     ),
    MAP(VC1,         VC1_COMPLEX,     VC1Advanced ),
    MAP(VC1,         VC1_ADVANCED,    VC1Advanced ),
    MAP(H263,        UNKNOWN,         H263Baseline),
    MAP(MPEG4,       MPEG4_SIMPLE,    MPEG4Simple ),
    MAP(MPEG4,       MPEG4_ADVANCED_SIMPLE, MPEG4AdvancedSimple),
    MAP(MPEG4,       MPEG4_MAIN,      MPEG4Main   ),
#if VA_CHECK_VERSION(0, 35, 0)
    MAP(VP8,         UNKNOWN,       VP8Version0_3 ),
#endif
#if VA_CHECK_VERSION(0, 37, 1)
    MAP(VP9,         VP9_0,           VP9Profile0 ),
#endif

#undef MAP
 };

/*
 From the va.h, should  support all?

    VAProfileNone                       = -1,
    VAProfileMPEG2Simple                = 0,
    VAProfileMPEG2Main                  = 1,
    VAProfileMPEG4Simple                = 2,
    VAProfileMPEG4AdvancedSimple        = 3,
    VAProfileMPEG4Main                  = 4,
    VAProfileH264Baseline va_deprecated_enum = 5,
    VAProfileH264Main                   = 6,
    VAProfileH264High                   = 7,
    VAProfileVC1Simple                  = 8,
    VAProfileVC1Main                    = 9,
    VAProfileVC1Advanced                = 10,
    VAProfileH263Baseline               = 11,
    VAProfileJPEGBaseline               = 12,
    VAProfileH264ConstrainedBaseline    = 13,
    VAProfileVP8Version0_3              = 14,
    VAProfileH264MultiviewHigh          = 15,
    VAProfileH264StereoHigh             = 16,
    VAProfileHEVCMain                   = 17,
    VAProfileHEVCMain10                 = 18,
    VAProfileVP9Profile0                = 19,
    VAProfileVP9Profile1                = 20,
    VAProfileVP9Profile2                = 21,
    VAProfileVP9Profile3                = 22,
    VAProfileHEVCMain12                 = 23,
    VAProfileHEVCMain422_10             = 24,
    VAProfileHEVCMain422_12             = 25,
    VAProfileHEVCMain444                = 26,
    VAProfileHEVCMain444_10             = 27,
    VAProfileHEVCMain444_12             = 28,
    VAProfileHEVCSccMain                = 29,
    VAProfileHEVCSccMain10              = 30,
    VAProfileHEVCSccMain444             = 31,
    VAProfileAV1Profile0                = 32,
    VAProfileAV1Profile1                = 33,
    VAProfileHEVCSccMain444_10          = 34

*/

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

void CFFUtil::load_ffmpeg(void){
#ifdef DEBUG
    av_log_set_level(AV_LOG_DEBUG);
#else
    av_log_set_level(AV_LOG_WARNING);
#endif
    av_log_set_callback(ffmpeg_log_callback);

    //init vaapi
    vaapi_ctx = NULL;
    //we need to always try to load to avoid calling them avcodec_open2() which will cause a deadlock
    load_vaapi();
    load_vdpau();
}

void CFFUtil::load_vaapi(void){
    if (vaapi_ctx || vaapi_failed){
        return;//quick check, return if not required
    }
    lock();//lock and re-check (for races)
    if (vaapi_ctx || vaapi_failed){
        unlock();
        return;//don't double alloc it
    }

    //FIXME: these NULLs should be user options
    int ret = av_hwdevice_ctx_create(&vaapi_ctx, AV_HWDEVICE_TYPE_VAAPI, NULL, NULL, 0);
    if (ret < 0) {
        vaapi_ctx = NULL;
        vaapi_failed = true;
        unlock();
        return;
    }
    unlock();
}

void CFFUtil::free_vaapi(void){
    lock();
    av_buffer_unref(&vaapi_ctx);
    vaapi_failed = false;
    unlock();
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


void CFFUtil::load_vdpau(void){
    if (vdpau_ctx || vdpau_failed){
        return;//quick check
    }
    lock();//recheck holding the lock
    if (vdpau_ctx || vdpau_failed){
        unlock();
        return;//don't double alloc it
    }

    //FIXME: these NULLs should be user options
    int ret = av_hwdevice_ctx_create(&vdpau_ctx, AV_HWDEVICE_TYPE_VDPAU, NULL, NULL, 0);
    if (ret < 0) {
        vdpau_ctx = NULL;
        vdpau_failed = true;
        unlock();
        return;
    }
    unlock();
}

void CFFUtil::free_vdpau(void){
    lock();
    av_buffer_unref(&vdpau_ctx);
    vdpau_failed = false;
    unlock();
}

//ripped from vdpau_transcode.c ffmpeg demo
enum AVPixelFormat get_vdpau_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts) {
    const enum AVPixelFormat *p;
    for (p = pix_fmts; *p != AV_PIX_FMT_NONE; p++){
        if (*p == AV_PIX_FMT_VDPAU){
            return *p;
        }
    }
    return AV_PIX_FMT_NONE;//should this be pix_fmts?
}

//Modified list, based on ffmpeg's hwcontext_vaapi.c
static const AVPixelFormat vaapi_format_map[] = {
    AV_PIX_FMT_NV12,
    AV_PIX_FMT_YUV420P,
    AV_PIX_FMT_YUV422P,
    AV_PIX_FMT_UYVY422,
    AV_PIX_FMT_YUYV422,
#ifdef VA_FOURCC_Y210
#ifdef AV_PIX_FMT_Y210
    AV_PIX_FMT_Y210,
#endif
#endif
    AV_PIX_FMT_YUV411P,
    AV_PIX_FMT_YUV440P,
    AV_PIX_FMT_YUV444P,
    AV_PIX_FMT_GRAY8,
#ifdef VA_FOURCC_P010
#ifdef AV_PIX_FMT_P010
    AV_PIX_FMT_P010,
#endif
#endif
    AV_PIX_FMT_BGRA,
    AV_PIX_FMT_BGR0,
    AV_PIX_FMT_RGBA,
    AV_PIX_FMT_RGB0,
#ifdef VA_FOURCC_ABGR
#ifdef AV_PIX_FMT_ABGR
    AV_PIX_FMT_ABGR,
#endif
#ifdef AV_PIX_FMT_0BGR
    AV_PIX_FMT_0BGR,
#endif
#endif
    AV_PIX_FMT_ARGB,
    AV_PIX_FMT_0RGB,
#ifdef VA_FOURCC_X2R10G10B10
#ifdef AV_PIX_FMT_X2RGB10
    AV_PIX_FMT_X2RGB10,
#endif
#endif
    AV_PIX_FMT_NONE
};


//check if we have a VAAPI compatible format to use, use the list from hwcontext_vaapi.c
enum AVPixelFormat get_sw_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts) {
    const enum AVPixelFormat *p;
    AVPixelFormat first = pix_fmts[0];
#ifdef HAVE_VAAPI
    for (p = pix_fmts; *p != AV_PIX_FMT_NONE; p++){
        if (gcff_util.sw_format_is_hw_compatable(*p)){
            return *p;
        }
    }
    LDEBUG("No VAAPI compatble pixel formats available for decoder");
#endif
    return first;//just whatever...
}

bool CFFUtil::sw_format_is_hw_compatable(const enum AVPixelFormat pix_fmt){
    load_vaapi();
    if (!vaapi_ctx){
        return false;
    }

    AVHWFramesConstraints* c = av_hwdevice_get_hwframe_constraints(vaapi_ctx, NULL);
    if (!c){
        return false;
    }

    const enum AVPixelFormat *v;
    for (v = c->valid_sw_formats; *v != AV_PIX_FMT_NONE; v++){
        if (*v == pix_fmt){
            av_hwframe_constraints_free(&c);
            return true;
        }
    }
    av_hwframe_constraints_free(&c);
    return false;
}

std::string CFFUtil::get_sw_hw_format_list(Config &cfg){
    load_vaapi();
    if (!vaapi_ctx){
        return "";
    }
    const std::string &cfg_fmt = cfg.get_value("video-hw-pix-fmt");
    if (cfg_fmt != "auto"){
        if (av_get_pix_fmt(cfg_fmt.c_str()) != AV_PIX_FMT_NONE){
            return std::string(cfg_fmt);
        }
    }
    AVHWFramesConstraints* c = av_hwdevice_get_hwframe_constraints(vaapi_ctx, NULL);
    if (!c){
        return "";
    }

    std::stringstream ss;
    const enum AVPixelFormat *v;
    for (v = c->valid_sw_formats; *v != AV_PIX_FMT_NONE; v++){
        if (v != c->valid_sw_formats){
            ss << "|";
        }
        ss << av_get_pix_fmt_name(*v);
    }
    av_hwframe_constraints_free(&c);
    return ss.str();
}
bool CFFUtil::have_vaapi(AVCodecID codec_id, int codec_profile, int width, int height){
    load_vaapi();
    if (!vaapi_ctx){
        return NULL;
    }

    //this section is only used to see if VAAPI supports our codec, without VAAPI we can still ask libavcodec to do it, but we won't known
    //if it's going to work, this will result in is just failing if we try it and it's not supported
#ifdef HAVE_VAAPI
    AVVAAPIDeviceContext* hwctx = reinterpret_cast<AVVAAPIDeviceContext*>((reinterpret_cast<AVHWDeviceContext*>(vaapi_ctx->data))->hwctx);
    //query VAAPI profiles for this codec
    const AVCodecDescriptor *codec_desc;
    codec_desc = avcodec_descriptor_get(codec_id);
    if (!codec_desc) {
        return NULL;
    }
    VAProfile profile, *profile_list = NULL;
    int profile_count;
    profile_count = vaMaxNumProfiles(hwctx->display);
    profile_list = new VAProfile[profile_count];
    if (!profile_list){
        return NULL;
    }

    VAStatus vas;
    vas = vaQueryConfigProfiles(hwctx->display, profile_list, &profile_count);
    if (vas != VA_STATUS_SUCCESS) {
        delete[] profile_list;
        return NULL;
    }

    profile = VAProfileNone;

    //search all known codecs to see if there is a matching profile
    for (unsigned int i = 0; i < FF_ARRAY_ELEMS(vaapi_profile_map); i++) {
        if (codec_id != vaapi_profile_map[i].codec_id){
            continue;
        }
        for (int j = 0; j < profile_count; j++) {
            if (vaapi_profile_map[i].va_profile == profile_list[j] &&
                vaapi_profile_map[i].codec_profile == codec_profile) {
                profile = profile_list[j];
                break;//exact found, we are done
            } else if (vaapi_profile_map[i].va_profile == profile_list[j]){
                profile = profile_list[j];//in exact found
            }
        }
    }
    delete[] profile_list;

    //if we couldn't find a matching profile we bail
    if (profile == VAProfileNone){
        LINFO("VA-API does not support this codec, no profile found");
        return NULL;
    }
#endif

    bool found = false;
    AVHWFramesConstraints* c = av_hwdevice_get_hwframe_constraints(vaapi_ctx, NULL);
    if (!c){
        return NULL;
    }
    //check if this is supported
    if (c->min_width  <= width || c->max_width  >= width ||
        c->min_height <= height || c->max_height >= height){
        //we assume the pixel formats are ok for us
        found = true;
    }
    av_hwframe_constraints_free(&c);
    return found;
}

AVBufferRef *CFFUtil::get_vaapi_ctx(AVCodecID codec_id, int codec_profile, int width, int height){
    if (have_vaapi(codec_id, codec_profile, width, height)){
        return av_buffer_ref(vaapi_ctx);
    } else {
        //not supported
        return NULL;
    }
}


bool CFFUtil::have_vdpau(AVCodecID codec_id, int codec_profile, int width, int height){
    load_vdpau();
    if (!vdpau_ctx){
        return NULL;
    }

#ifdef HAVE_VDPAU
    //Check if VDPAU supports this codec
    AVVDPAUDeviceContext* hwctx = reinterpret_cast<AVVDPAUDeviceContext*>((reinterpret_cast<AVHWDeviceContext*>(vdpau_ctx->data))->hwctx);
    uint32_t max_width, max_height;
    VdpBool supported;
    VdpDecoderProfile vdpau_profile;
    //get  profile
    int ret = get_vdpau_profile(codec_id, codec_profile, &vdpau_profile);
    if (ret){
        //something didn't work, don't use VDPAU
        return NULL;
    }
    VdpDecoderQueryCapabilities *vdpau_query_caps;
    VdpStatus status = hwctx->get_proc_address(hwctx->device, VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES, reinterpret_cast<void**>(&vdpau_query_caps));
    if (status != VDP_STATUS_OK){
        return NULL;
    }
    status = vdpau_query_caps(hwctx->device, vdpau_profile, &supported, NULL, NULL, &max_width, &max_height);
    if (status != VDP_STATUS_OK){
        return NULL;
    }

    if (supported != VDP_TRUE){
        return NULL;
    }

    if (static_cast<uint32_t>(width) > max_width || static_cast<uint32_t>(height) > max_height){
        return NULL;
    }
#endif
    bool found = false;
    AVHWFramesConstraints* c = av_hwdevice_get_hwframe_constraints(vdpau_ctx, NULL);
    //check if this is supported
    if (c->min_width  <= width || c->max_width  >= width ||
        c->min_height <= height || c->max_height >= height){
        //we assume the pixel formats are ok for us
        found = true;
    }

    av_hwframe_constraints_free(&c);
    return found;
}

AVBufferRef *CFFUtil::get_vdpau_ctx(AVCodecID codec_id, int codec_profile, int width, int height){
    if (have_vdpau(codec_id, codec_profile, width, height)){
        return av_buffer_ref(vdpau_ctx);
    } else {
        //not supported
        return NULL;
    }

}

CFFUtil::CFFUtil(void){
    vaapi_failed = false;
    vdpau_failed = false;
}

CFFUtil::~CFFUtil(void){
    free_hw();
}

void CFFUtil::lock(void){
    codec_lock.lock();
}

void CFFUtil::unlock(void){
    codec_lock.unlock();
}

void CFFUtil::free_hw(void){
    free_vdpau();
    free_vaapi();
}

#ifdef HAVE_VDPAU
//borrowed from FFMPeg's vdpau.c, deprecated so we'll do it ourselves
int CFFUtil::get_vdpau_profile(const AVCodecID codec_id, const int codec_profile, VdpDecoderProfile *profile){
#define PROFILE(prof)                           \
    do {                                        \
        *profile = VDP_DECODER_PROFILE_##prof;  \
        return 0;                               \
    } while (0)

    switch (codec_id) {
    case AV_CODEC_ID_MPEG1VIDEO:               PROFILE(MPEG1);
    case AV_CODEC_ID_MPEG2VIDEO:
        switch (codec_profile) {
        case FF_PROFILE_MPEG2_MAIN:            PROFILE(MPEG2_MAIN);
        case FF_PROFILE_MPEG2_SIMPLE:          PROFILE(MPEG2_SIMPLE);
        default:                               return AVERROR(EINVAL);
        }
    case AV_CODEC_ID_H263:                     PROFILE(MPEG4_PART2_ASP);
    case AV_CODEC_ID_MPEG4:
        switch (codec_profile) {
        case FF_PROFILE_MPEG4_SIMPLE:          PROFILE(MPEG4_PART2_SP);
        case FF_PROFILE_MPEG4_ADVANCED_SIMPLE: PROFILE(MPEG4_PART2_ASP);
        default:                               return AVERROR(EINVAL);
        }
    case AV_CODEC_ID_H264:
        switch (codec_profile & ~FF_PROFILE_H264_INTRA) {
        case FF_PROFILE_H264_BASELINE:         PROFILE(H264_BASELINE);
        case FF_PROFILE_H264_CONSTRAINED_BASELINE:
        case FF_PROFILE_H264_MAIN:             PROFILE(H264_MAIN);
        case FF_PROFILE_H264_HIGH:             PROFILE(H264_HIGH);
#ifdef VDP_DECODER_PROFILE_H264_EXTENDED
        case FF_PROFILE_H264_EXTENDED:         PROFILE(H264_EXTENDED);
#endif
        default:                               return AVERROR(EINVAL);
        }
    case AV_CODEC_ID_WMV3:
    case AV_CODEC_ID_VC1:
        switch (codec_profile) {
        case FF_PROFILE_VC1_SIMPLE:            PROFILE(VC1_SIMPLE);
        case FF_PROFILE_VC1_MAIN:              PROFILE(VC1_MAIN);
        case FF_PROFILE_VC1_ADVANCED:          PROFILE(VC1_ADVANCED);
        default:                               return AVERROR(EINVAL);
        }
    default:
        return AVERROR(EINVAL);
    }
    return AVERROR(EINVAL);
}
#endif

AVBufferRef *CFFUtil::get_opencl_ctx(AVCodecID codec_id, int codec_profile, int width, int height){
    if (have_opencl(codec_id, codec_profile, width, height)){
        return av_buffer_ref(opencl_ctx);
    } else {
        //not supported
        return NULL;
    }
}

bool CFFUtil::have_opencl(AVCodecID codec_id, int codec_profile, int width, int height){
    load_vaapi();
    if (!vaapi_ctx){
        return NULL;
    }

    //this section is only used to see if VAAPI supports our codec, without VAAPI we can still ask libavcodec to do it, but we won't known
    //if it's going to work, this will result in is just failing if we try it and it's not supported
#ifdef HAVE_VAAPI
    AVVAAPIDeviceContext* hwctx = reinterpret_cast<AVVAAPIDeviceContext*>((reinterpret_cast<AVHWDeviceContext*>(vaapi_ctx->data))->hwctx);
    //query VAAPI profiles for this codec
    const AVCodecDescriptor *codec_desc;
    codec_desc = avcodec_descriptor_get(codec_id);
    if (!codec_desc) {
        return NULL;
    }
    VAProfile profile, *profile_list = NULL;
    int profile_count;
    profile_count = vaMaxNumProfiles(hwctx->display);
    profile_list = new VAProfile[profile_count];
    if (!profile_list){
        return NULL;
    }

    VAStatus vas;
    vas = vaQueryConfigProfiles(hwctx->display, profile_list, &profile_count);
    if (vas != VA_STATUS_SUCCESS) {
        delete[] profile_list;
        return NULL;
    }

    profile = VAProfileNone;

    //search all known codecs to see if there is a matching profile
    for (unsigned int i = 0; i < FF_ARRAY_ELEMS(vaapi_profile_map); i++) {
        if (codec_id != vaapi_profile_map[i].codec_id){
            continue;
        }
        for (int j = 0; j < profile_count; j++) {
            if (vaapi_profile_map[i].va_profile == profile_list[j] &&
                vaapi_profile_map[i].codec_profile == codec_profile) {
                profile = profile_list[j];
                break;//exact found, we are done
            } else if (vaapi_profile_map[i].va_profile == profile_list[j]){
                profile = profile_list[j];//in exact found
            }
        }
    }
    delete[] profile_list;

    //if we couldn't find a matching profile we bail
    if (profile == VAProfileNone){
        LINFO("VA-API does not support this codec, no profile found");
        return NULL;
    }
#endif

    bool found = false;
    AVHWFramesConstraints* c = av_hwdevice_get_hwframe_constraints(vaapi_ctx, NULL);
    if (!c){
        return NULL;
    }
    //check if this is supported
    if (c->min_width  <= width || c->max_width  >= width ||
        c->min_height <= height || c->max_height >= height){
        //we assume the pixel formats are ok for us
        found = true;
    }
    av_hwframe_constraints_free(&c);
    return found;
}

void CFFUtil::load_opencl(void){
    if (opencl_ctx || opencl_failed || vaapi_failed){
        return;//quick check, return if not required
    }
    load_vaapi();//vaapi is required for opencl as we derive it from our opencl instance
    lock();//lock and re-check (for races)
    if (opencl_ctx || opencl_failed || !vaapi_ctx || vaapi_failed){
        unlock();
        return;//don't double alloc it
    }

    //this maps opencl to vaapi
    int ret = av_hwdevice_ctx_create_derived(&opencl_ctx, AV_HWDEVICE_TYPE_OPENCL, vaapi_ctx, 0);
    if (ret < 0) {
        opencl_ctx = NULL;
        opencl_failed = true;
        unlock();
        return;
    }

     //this maps opencv to opencl
#if defined(HAVE_OPENCV) && defined(HAVE_OPENCL)
    //opencv must use this context
    AVOpenCLDeviceContext * ocl_device_ctx = get_opencl_ctx_from_device(opencl_ctx);
    size_t param_value_size;

    //Get context properties
    clGetContextInfo(ocl_device_ctx->context, CL_CONTEXT_PROPERTIES, 0, NULL, &param_value_size);
    std::vector<cl_context_properties> props(param_value_size/sizeof(cl_context_properties));
    clGetContextInfo(ocl_device_ctx->context, CL_CONTEXT_PROPERTIES, param_value_size, props.data(), NULL);

    //Find the platform prop
    cl_platform_id platform = 0;
    for (int i = 0; props[i] != 0; i = i + 2) {
        if (props[i] == CL_CONTEXT_PLATFORM) {
            platform = reinterpret_cast<cl_platform_id>(props[i + 1]);
        }
    }

    // Get the name for the platform
    clGetPlatformInfo(platform, CL_PLATFORM_NAME, 0, NULL, &param_value_size);
    std::vector <char> platform_name(param_value_size);
    clGetPlatformInfo(platform, CL_PLATFORM_NAME, param_value_size, platform_name.data(), NULL);

    //Finally: attach the context to OpenCV
    cv::ocl::attachContext(platform_name.data(), platform, ocl_device_ctx->context, ocl_device_ctx->device_id);
#endif
    unlock();
}

void CFFUtil::free_opencl(void){
    lock();
    av_buffer_unref(&opencl_ctx);
    opencl_failed = false;
    unlock();
}


AVVAAPIDeviceContext *get_vaapi_ctx_from_device(AVBufferRef *buf){
    if (!buf || !buf->data){
        return NULL;
    }
    AVHWDeviceContext *device = reinterpret_cast<AVHWDeviceContext*>(buf->data);
    if (device->type != AV_HWDEVICE_TYPE_VAAPI){
        return NULL;
    }
    return static_cast<AVVAAPIDeviceContext*>(device->hwctx);
}

AVVAAPIDeviceContext *get_vaapi_ctx_from_frames(AVBufferRef *buf){
    if (!buf || !buf->data){
        return NULL;
    }
    AVHWFramesContext *frames = reinterpret_cast<AVHWFramesContext*>(buf->data);
    return get_vaapi_ctx_from_device(frames->device_ref);
}
#ifdef HAVE_OPENCL
AVOpenCLDeviceContext *get_opencl_ctx_from_device(AVBufferRef *buf){
    if (!buf || !buf->data){
        return NULL;
    }
    AVHWDeviceContext *device = reinterpret_cast<AVHWDeviceContext*>(buf->data);
    if (device->type != AV_HWDEVICE_TYPE_OPENCL){
        return NULL;
    }
    return static_cast<AVOpenCLDeviceContext*>(device->hwctx);
}
#endif

#ifdef HAVE_OPENCL
AVOpenCLDeviceContext *get_opencl_ctx_from_frames(AVBufferRef *buf){
    if (!buf || !buf->data){
        return NULL;
    }
    AVHWFramesContext *frames = reinterpret_cast<AVHWFramesContext*>(buf->data);
    return get_opencl_ctx_from_device(frames->device_ref);
}
#endif


bool CFFUtil::have_v4l2(AVCodecID codec_id, int codec_profile, int width, int height){
#ifdef HAVE_V4L2
    //we need to check all devices
    DIR *dirp = opendir("/dev");
    if (!dirp){
        LWARN("Can't open /dev");
        return false;
    }

    bool ret = false;
    struct dirent *entry;
    while (!ret && (entry = readdir(dirp))){
        std::string dev_name = std::string(entry->d_name);
        if (dev_name.find("video") != 0){
            continue;
        }
        int fd = v4l2_open(std::string("/dev/" + dev_name).c_str(), O_RDWR | O_NONBLOCK, 0);
        if (fd < 0){
            LWARN("Cannot open /dev/" + dev_name + " (" + std::to_string(errno) + ")");
            continue;
        }
        ret = have_v4l2_dev(fd, codec_id, codec_profile, width, height);
        if (v4l2_close(fd)){
            LWARN("close() failed (" + std::to_string(errno) + ")");
        }
    }
    closedir(dirp);
    LDEBUG("Completed dev check");
    return ret;
    //query pixel formats first
#else
    return false;//no v4l2 enabled
#endif
}

#ifdef HAVE_V4L2
bool CFFUtil::have_v4l2_dev(int dev, AVCodecID codec_id, int codec_profile, int width, int height){
    bool ret = false;

    //check if it supports M2M
    struct v4l2_capability caps;
    if (!vioctl(dev, VIDIOC_QUERYCAP, &caps)){
        return false;
    }

    LDEBUG("Found V4L2 device " + std::string(reinterpret_cast<char*>(caps.card)) + "[" + std::string(reinterpret_cast<char*>(caps.bus_info)) + "]");

    bool splane = caps.device_caps & V4L2_CAP_VIDEO_M2M;
    bool mplane = caps.device_caps & V4L2_CAP_VIDEO_M2M_MPLANE;
    if (!splane && !mplane){
        LDEBUG("...Does not support M2M");
        return false;
    }

    struct v4l2_fmtdesc pixfmt;
    for (int pix_idx = 0;; pix_idx++){
        pixfmt.index = pix_idx;
        pixfmt.type = splane ? V4L2_BUF_TYPE_VIDEO_OUTPUT : V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        if (!vioctl(dev, VIDIOC_ENUM_FMT, &pixfmt)){
            if (splane && mplane){
                //not sure if anyone supports both on the same driver, but we do!
                //this repeates the loop for mplane
                splane = false;
                pix_idx = -1;
                continue;
            }
            break;//hit the end
        }

        if (pixfmt.flags & V4L2_FMT_FLAG_EMULATED){
            continue;//we don't care about SW formats
        }

        if (!is_v4l2_hw_codec(codec_id, pixfmt.pixelformat)){
            continue;//now the requested codec, irrelevant
        }
        struct v4l2_frmsizeenum frsize;
        for (int fr_idx = 0; ; fr_idx++){
            frsize.index = fr_idx;
            frsize.pixel_format = pixfmt.pixelformat;
            if (!vioctl(dev, VIDIOC_ENUM_FRAMESIZES, &frsize)){
                break;
            }
            if (frsize.type == V4L2_FRMSIZE_TYPE_DISCRETE){
                if (width == frsize.discrete.width && height == frsize.discrete.height){
                    return true;
                }
                break;//DISCRETE is only one
            } else if (frsize.type == V4L2_FRMSIZE_TYPE_STEPWISE || frsize.type == V4L2_FRMSIZE_TYPE_CONTINUOUS) {
                if (width >= frsize.stepwise.min_width &&
                    width <= frsize.stepwise.max_width &&
                    width % frsize.stepwise.step_width == 0 &&
                    height >= frsize.stepwise.min_height &&
                    height <= frsize.stepwise.max_height &&
                    height % frsize.stepwise.step_height == 0){
                    return true;
                }
            } else {
                LWARN("Unknown V4L2 Type - " + std::to_string(frsize.type));
                return false;
            }
        }
    }
    return ret;
}
#endif

#ifdef HAVE_V4L2
bool CFFUtil::vioctl(int fh, int request, void *arg){
    int r;

    do {
        r = v4l2_ioctl(fh, request, arg);
    } while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));

    if (r == -1){
        return false;
    }
    return true;
}
#endif

#ifdef HAVE_V4L2
bool CFFUtil::is_v4l2_hw_codec(const AVCodecID av_codec, const uint32_t v4l2_pix_fmt){
    //i hate these things
    //we only use this to determine if FFMpeg will probably succeed, so it doesn't have to be 100% accurate
    switch (av_codec){
    case AV_CODEC_ID_MPEG1VIDEO:
        return v4l2_pix_fmt == V4L2_PIX_FMT_MPEG1;
    case AV_CODEC_ID_MPEG2VIDEO:
        return v4l2_pix_fmt == V4L2_PIX_FMT_MPEG2;
    case AV_CODEC_ID_H263:
        return v4l2_pix_fmt == V4L2_PIX_FMT_H263;
    case AV_CODEC_ID_H264:
        return v4l2_pix_fmt == V4L2_PIX_FMT_H264 ||
            v4l2_pix_fmt == V4L2_PIX_FMT_H264_NO_SC ||
            v4l2_pix_fmt == V4L2_PIX_FMT_H264_MVC;
    case AV_CODEC_ID_MPEG4:
        return v4l2_pix_fmt == V4L2_PIX_FMT_MPEG4 ||
            v4l2_pix_fmt == V4L2_PIX_FMT_XVID;
    case AV_CODEC_ID_VC1:
        return v4l2_pix_fmt == V4L2_PIX_FMT_VC1_ANNEX_G ||
            v4l2_pix_fmt == V4L2_PIX_FMT_VC1_ANNEX_L;
    case AV_CODEC_ID_HEVC:
        return v4l2_pix_fmt == V4L2_PIX_FMT_HEVC;
    case AV_CODEC_ID_VP8:
        return v4l2_pix_fmt == V4L2_PIX_FMT_VP8;
    case AV_CODEC_ID_VP9:
        return v4l2_pix_fmt == V4L2_PIX_FMT_VP9;
    default:
        return false;
    }
}
#endif
