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
 *   Copyright 2022 Ed Martin <edman007@edman007.com>
 *
 **************************************************************************
 */

#include "motion_opencv.hpp"
#ifdef HAVE_OPENCV
#include "util.hpp"

#ifdef HAVE_VAAPI
#include <opencv2/core/va_intel.hpp>
#endif

#define CL_TARGET_OPENCL_VERSION 300
#ifdef HAVE_OPENCL
#ifdef HAVE_OPENCL_CL_H
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif
#endif

#include <opencv2/imgproc.hpp>

//debug things
#include <opencv2/highgui.hpp>
#include "image_util.hpp"


static const std::string algo_name = "opencv";

MotionOpenCV::MotionOpenCV(Config &cfg, Database &db, MotionController &controller) : MotionAlgo(cfg, db, controller, algo_name), fmt_filter(Filter(cfg)) {
    input = av_frame_alloc();
    map_indirect = cfg.get_value("motion-opencv-map-indirect") != "false";//if true will use libva to copy the brighness
    map_cl = cfg.get_value("motion-opencv-map-cl") != "false";//use ffmpeg to convert to opencl and then map that into opencv
    map_vaapi = cfg.get_value("motion-opencv-map-vaapi") != "false";//use OpenCV's VA-API mapping implementation
}

MotionOpenCV::~MotionOpenCV(){
    av_frame_free(&input);
}
bool MotionOpenCV::process_frame(const AVFrame *frame, bool video){
    if (!video){
        return true;
    }
    buf_mat.release();
    input_mat.release();
    //opencv libva is HAVE_VA, our libva is HAVE_VAAPI
    //while it might be built without va-api, we can detect is runtime, so map_vaapi will be false if support is missing
    //check if the incoming frame is VAAPI
    if (map_vaapi && frame->format == AV_PIX_FMT_VAAPI && frame->hw_frames_ctx){
        AVVAAPIDeviceContext *hwctx = get_vaapi_ctx_from_frames(frame->hw_frames_ctx);;
        if (!hwctx){
            LERROR("VAAPI frame does not have VAAPI device");
            return false;
        }

        const VASurfaceID surf = reinterpret_cast<uintptr_t const>(frame->data[3]);
        try {
            cv::va_intel::convertFromVASurface(hwctx->display, surf, cv::Size(frame->width, frame->height), tmp1);
            //change to CV_8UC1
            if (tmp1.depth() == CV_8U){
                cv::cvtColor(tmp1, buf_mat, cv::COLOR_BGR2GRAY);
            } else {
                double alpha = 1.0;
                if (tmp1.depth() == CV_16U){
                    alpha = 1.0/256;
                } else {
                    LWARN("convertFromVASurface provided unsupported depth, not scaling it");
                }
                tmp1.convertTo(tmp2, CV_8U, alpha);
                cv::cvtColor(tmp2, buf_mat, cv::COLOR_BGR2GRAY);
            }
        } catch (cv::Exception &e){
            LWARN("Error converting image from VA-API To OpenCV: " + e.msg);
            return false;
        }
    } else
#ifdef HAVE_VAAPI
    //this is a higher speed indirect (no-interopt) version for non-intel HW
    if (map_indirect && frame->format == AV_PIX_FMT_VAAPI && frame->hw_frames_ctx){
        indirect_vaapi_map(frame);
    } else
#endif
#ifdef HAVE_OPENCL
    if (map_cl && frame->format == AV_PIX_FMT_VAAPI && frame->hw_frames_ctx){
        //map it to openCL
        if (!fmt_filter.send_frame(frame)){
            LWARN("OpenCV Failed to Send Frame");
            return false;
        }
        av_frame_unref(input);
        if (!fmt_filter.get_frame(input)){
            LWARN("OpenCV Failed to Get Frame");
            return false;
        }
        map_ocl_frame(input);
    } else
#endif
    {
        //direct VAAPI conversion not possible
        if (!fmt_filter.send_frame(frame)){
            LWARN("OpenCV Failed to Send Frame");
            return false;
        }
        av_frame_unref(input);
        if (!fmt_filter.get_frame(input)){
            LWARN("OpenCV Failed to Get Frame");
            return false;
        }
        //CV_16UC1 matches the format in set_video_stream()
        input_mat = cv::Mat(input->height, input->width, CV_8UC1, input->data[0], input->linesize[0]);
        input_mat.copyTo(buf_mat);
        //buf_mat = input_mat.getUMat(cv::ACCESS_READ);

    }

    return true;
}

bool MotionOpenCV::set_video_stream(const AVStream *stream, const AVCodecContext *codec) {
    //if the codec does not have VA-API then we need to not use opencl
    if (codec && codec->hw_device_ctx){
        //find the VAAPI Display
        AVVAAPIDeviceContext* hw_ctx = get_vaapi_ctx_from_device(codec->hw_device_ctx);
        if (map_vaapi && hw_ctx){
            //bind this display to this thread
            try {
                cv::va_intel::ocl::initializeContextFromVA(hw_ctx->display, true);
            } catch (cv::Exception &e){
                //if we actually don't have va-api, this will fail
                LWARN("cv::va_intel::ocl::initializeContextFromVA failed, disable this by setting motion-opencv-map-vaapi to false, error: " + e.msg);
                map_vaapi = false;
            }
        } else if (!hw_ctx){
            LINFO("No VA-API Context");
            map_cl = false;
        }
    } else {
        LINFO("No HW Context");
        if (!codec){
            LWARN("No decode codec passed to MotionOpenCV!");
        }
        map_cl = false;
    }

    //CODEC is only needed for HW Mapping...we will map it ourself
    fmt_filter.set_source_time_base(stream->time_base);
    if (map_cl){
        LWARN("Setup for OpenCL");
        fmt_filter.set_target_fmt(AV_PIX_FMT_OPENCL, AV_CODEC_ID_NONE, 0);//
    } else {
        fmt_filter.set_target_fmt(AV_PIX_FMT_GRAY8, AV_CODEC_ID_NONE, 0);
    }
    return true;
}

const std::string& MotionOpenCV::get_mod_name(void) {
    return algo_name;
}

const cv::UMat& MotionOpenCV::get_UMat(void){
    return buf_mat;
}

#ifdef HAVE_OPENCL
void MotionOpenCV::map_ocl_frame(AVFrame *input){
    //Extract the two OpenCL Image2Ds from the opencl frame
    cl_mem luma_image = reinterpret_cast<cl_mem>(input->data[0]);
    cl_mem chrome_image = reinterpret_cast<cl_mem>(input->data[1]);

    size_t luma_w = 0;
    size_t luma_h = 0;
    size_t chroma_w = 0;
    size_t chroma_h = 0;

    clGetImageInfo(luma_image, CL_IMAGE_WIDTH, sizeof(size_t), &luma_w, 0);
    clGetImageInfo(luma_image, CL_IMAGE_HEIGHT, sizeof(size_t), &luma_h, 0);
    clGetImageInfo(chrome_image, CL_IMAGE_WIDTH, sizeof(size_t), &chroma_w, 0);
    clGetImageInfo(chrome_image, CL_IMAGE_HEIGHT, sizeof(size_t), &chroma_h, 0);


    //FIXME: assumes this is NV12
    tmp1.create(luma_h + chroma_h, luma_w, CV_8U);

    cl_mem dst_buffer = static_cast<cl_mem>(tmp1.handle(cv::ACCESS_READ));
    cl_command_queue queue = static_cast<cl_command_queue>(cv::ocl::Queue::getDefault().ptr());
    size_t src_origin[3] = { 0, 0, 0 };
    size_t luma_region[3] = { luma_w, luma_h, 1 };
    size_t chroma_region[3] = { chroma_w, chroma_h * 2, 1 };

    //Copy the contents of each Image2Ds to the right place in the
    //OpenCL buffer which backs the Mat
    clEnqueueCopyImageToBuffer(queue, luma_image, dst_buffer, src_origin, luma_region, 0, 0, NULL, NULL);
    clEnqueueCopyImageToBuffer(queue, chrome_image, dst_buffer, src_origin, chroma_region, luma_w * luma_h * 1, 0, NULL, NULL);

    // Block until the copying is done
    clFinish(queue);

}
//HAVE_OPENCL
#endif

#ifdef HAVE_VAAPI
//inspired from OpenCVs convertFromvasurface
void MotionOpenCV::indirect_vaapi_map(const AVFrame *input){
    AVVAAPIDeviceContext* hw_ctx = get_vaapi_ctx_from_frames(input->hw_frames_ctx);
    if (!hw_ctx){
        LWARN("No VAAPI context found");
        return;
    }

    const VASurfaceID surface = reinterpret_cast<uintptr_t const>(input->data[3]);
    VAStatus status = 0;

    status = vaSyncSurface(hw_ctx->display, surface);
    if (status != VA_STATUS_SUCCESS){
        LWARN("vaSyncSurface: Failed");
        return;
    }

    VAImage image;
    status = vaDeriveImage(hw_ctx->display, surface, &image);
    if (status != VA_STATUS_SUCCESS){
        //try vaCreateImage + vaGetImage
        //pick a format
        int num_formats = vaMaxNumImageFormats(hw_ctx->display);
        if (num_formats <= 0){
            LWARN("VA-API: vaMaxNumImageFormats failed");
            return;
        }
        std::vector<VAImageFormat> fmt_list(num_formats);

        status = vaQueryImageFormats(hw_ctx->display, fmt_list.data(), &num_formats);
        if (status != VA_STATUS_SUCCESS){
            LWARN("VA-API: vaQueryImageFormats failed");
            return;
        }
        VAImageFormat *selected_format = nullptr;
        for (auto &fmt : fmt_list){
            if (fmt.fourcc == VA_FOURCC_NV12 || fmt.fourcc == VA_FOURCC_YV12){
                selected_format = &fmt;
                break;
            }
        }
        if (selected_format == nullptr){
            LWARN("VA-API: vaQueryImageFormats did not return a supported format");
            return;
        }

        status = vaCreateImage(hw_ctx->display, selected_format, input->width, input->height, &image);
        if (status != VA_STATUS_SUCCESS){
            LWARN("VA-API: vaCreateImage failed");
            return;
        }

        status = vaGetImage(hw_ctx->display, surface, 0, 0, input->width, input->height, image.image_id);
        if (status != VA_STATUS_SUCCESS){
            vaDestroyImage(hw_ctx->display, image.image_id);
            LWARN("VA-API: vaPutImage failed");
            return;
        }
    }

    unsigned char* buffer = 0;
    status = vaMapBuffer(hw_ctx->display, image.buf, (void **)&buffer);
    if (status != VA_STATUS_SUCCESS){
        LWARN("VA-API: vaMapBuffer failed");
    }

    if (image.format.fourcc == VA_FOURCC_NV12){
        input_mat = cv::Mat(input->height, input->width, CV_8UC1, buffer + image.offsets[0], image.pitches[0]);
    } else if (image.format.fourcc == VA_FOURCC_YV12) {
        input_mat = cv::Mat(input->height, input->width, CV_8UC1, buffer + image.offsets[0], image.pitches[0]);
    } else {
        LWARN("VA-API didn't return correct format");
        return;
    }
    input_mat.copyTo(buf_mat);//buf_mat is now the luma channel which is close enough for us
    input_mat.release();

    status = vaUnmapBuffer(hw_ctx->display, image.buf);
    if (status != VA_STATUS_SUCCESS){
        LWARN("VA-API: vaUnmapBuffer failed");
        return;
    }

    status = vaDestroyImage(hw_ctx->display, image.image_id);
    if (status != VA_STATUS_SUCCESS){
        LWARN("VA-API: vaDestroyImage failed");
        return;
    }

}


//HAVE_VAAPI
#endif
//HAVE_OPENCV
#endif
