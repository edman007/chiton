#include "stream_unwrap.hpp"
#include "util.hpp"
#include "chiton_ffmpeg.hpp"


StreamUnwrap::StreamUnwrap(Config& cfg) : cfg(cfg) {


}


bool StreamUnwrap::connect(void) {
    const std::string& url = cfg.get_value("video-url");
    if (!url.compare("")){
        Util::log_msg(LOG_ERROR, "Camera was not supplied with a URL" + url);
        return false;
    }
    load_avformat();

    AVCodecContext *avctx;
    AVCodec *input_codec;
    AVFormatContext *input_format_context = NULL;
    int error;
    /* Open the input file to read from it. */
    if ((error = avformat_open_input(&input_format_context, url.c_str(), NULL, NULL)) < 0) {
        Util::log_msg(LOG_ERROR, "Could not open camera url '" + url + "' (error '" + std::string(av_err2str(error)) +")n");
        input_format_context = NULL;
        return false;
    }

    /* Get information on the input file (number of streams etc.). */
    if ((error = avformat_find_stream_info(input_format_context, NULL)) < 0) {
        Util::log_msg(LOG_ERROR, "Could not open find stream info (error '" + std::string(av_err2str(error)) + "')n");
        avformat_close_input(&input_format_context);
        return error;
    }
    Util::log_msg(LOG_INFO, "Video Stream has " + std::to_string(input_format_context->nb_streams) + " streams");
    
    return true;
}

bool StreamUnwrap::open_output(void){
    

}
void StreamUnwrap::load_avformat(void){
    av_log_set_level(AV_LOG_DEBUG);
    //probably should call  av_log_set_callback	(	void(*)(void *, int, const char *, va_list) callback	)	
}
