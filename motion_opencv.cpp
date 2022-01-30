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

#include <opencv2/imgproc.hpp>

static const std::string algo_name = "opencv";

MotionOpenCV::MotionOpenCV(Config &cfg, Database &db, MotionController &controller) : MotionAlgo(cfg, db, controller), fmt_filter(Filter(cfg)) {
    input = av_frame_alloc();
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
#ifdef HAVE_VAAPI
    //OPENCV has VA support
    //check if the incoming frame is VAAPI
    if (video && frame->format == AV_PIX_FMT_VAAPI && frame->hw_frames_ctx){
        AVHWFramesContext *hw_frames = (AVHWFramesContext *)frame->hw_frames_ctx->data;
        AVHWDeviceContext *device_ctx = hw_frames->device_ctx;
        AVVAAPIDeviceContext *hwctx;
        if (device_ctx->type != AV_HWDEVICE_TYPE_VAAPI){
            LERROR("VAAPI frame does not have VAAPI device");
            return false;
        }
        hwctx = static_cast<AVVAAPIDeviceContext*>(device_ctx->hwctx);
        cv::UMat tmp1;
        const VASurfaceID surf = reinterpret_cast<uintptr_t const>(frame->data[3]);
        try {
            cv::va_intel::convertFromVASurface(hwctx->display, surf, cv::Size(frame->width, frame->height), tmp1);
            //change to CV_8UC1
            if (tmp1.depth() == CV_8U){
                cv::cvtColor(tmp1, buf_mat, cv::COLOR_BGR2GRAY);
            } else {
                cv::UMat tmp2;
                double alpha = 1.0;
                if (tmp1.depth() == CV_16U){
                    alpha = 1.0/256;
                } else {
                    LWARN("convertFromVASurface provided unsupported depth");
                }
                tmp1.convertTo(tmp2, CV_8U, alpha);
                cv::cvtColor(tmp2, buf_mat, cv::COLOR_BGR2GRAY);
            }

        } catch (cv::Exception &e){
            LWARN("Error converting image from VA-API To OpenCV: " + e.msg);
            return false;
        }
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
        input_mat = cv::Mat(input->height, input->width, CV_8UC1, input->data, input->linesize[0]);
        input_mat.copyTo(buf_mat);
        //buf_mat = input_mat.getUMat(cv::ACCESS_READ);
    }

    return true;
}

bool MotionOpenCV::set_video_stream(const AVStream *stream, const AVCodecContext *codec) {
    if (codec && codec->hw_frames_ctx){
        //find the VAAPI Display
        AVHWDeviceContext* device_ctx = reinterpret_cast<AVHWDeviceContext*>(codec->hw_frames_ctx->data);
        if (device_ctx && device_ctx->type == AV_HWDEVICE_TYPE_VAAPI){
            AVVAAPIDeviceContext* hw_ctx = static_cast<AVVAAPIDeviceContext*>(device_ctx->hwctx);
            //bind this display to this thread
            cv::va_intel::ocl::initializeContextFromVA(hw_ctx->display, true);
        }
    }

    //stream->
    //cv::utils::getConfigurationParameterBool("OPENCV_OPENCL_ENABLE_MEM_USE_HOST_PTR", true);
    //CODEC is only needed for HW Mapping...we will map it ourself
    fmt_filter.set_source_time_base(stream->time_base);
    fmt_filter.set_target_fmt(AV_PIX_FMT_GRAY8, AV_CODEC_ID_NONE, 0);
    return true;
}

const std::string& MotionOpenCV::get_name(void) {
    return algo_name;
}

const std::string& MotionOpenCVAllocator::get_name(void) {
    return algo_name;
}

const cv::UMat& MotionOpenCV::get_UMat(void){
    return buf_mat;
}

//HAVE_OPENCV
#endif
