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
#include "chiton_ffmpeg.hpp"
#include "util.hpp"
#include <cstdio>
#ifdef HAVE_VAAPI
#include <va/va.h>
#include <libavutil/hwcontext_vaapi.h>
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
    MAP(H264,        H264_BASELINE,   H264Baseline),
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
}

void CFFUtil::load_vaapi(void){
    lock();
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
    lock();
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

AVBufferRef *CFFUtil::get_vaapi_ctx(AVCodecContext* avctx, int w, int h){
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
    codec_desc = avcodec_descriptor_get(avctx->codec_id);
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
        if (avctx->codec_id != vaapi_profile_map[i].codec_id){
            continue;
        }
        for (int j = 0; j < profile_count; j++) {
            if (vaapi_profile_map[i].va_profile == profile_list[j]) {
                profile = profile_list[j];
                break;
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
    if (c->min_width  <= w || c->max_width  >= w ||
        c->min_height <= h || c->max_height >= h){
        //we assume the pixel formats are ok for us
        found = true;
    }
    av_hwframe_constraints_free(&c);
    if (found){
        return av_buffer_ref(vaapi_ctx);
    } else {
        //not supported
        return NULL;
    }
}

AVBufferRef *CFFUtil::get_vdpau_ctx(AVCodecContext* avctx, int w, int h){
    load_vdpau();
    if (!vdpau_ctx){
        return NULL;
    }
    bool found = false;
    AVHWFramesConstraints* c = av_hwdevice_get_hwframe_constraints(vdpau_ctx, NULL);
    //check if this is supported
    if (c->min_width  <= w || c->max_width  >= w ||
        c->min_height <= h || c->max_height >= h){
        //we assume the pixel formats are ok for us
        found = true;
    }

    av_hwframe_constraints_free(&c);
    if (found){
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
