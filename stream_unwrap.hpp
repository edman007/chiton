#ifndef __STREAM_UNWRAP_HPP__
#define __STREAM_UNWRAP_HPP__

#include "chiton_config.hpp"
#include "chiton_ffmpeg.hpp"

class StreamUnwrap {
    /*
     * Handles the Network half of the video
     *
     */
public:
    StreamUnwrap(Config& cfg);
    ~StreamUnwrap();
    
    bool connect(void);//returns true on success
    AVCodecContext* get_codec_context(void);
    AVFormatContext* get_format_context(void);
    unsigned int get_stream_count(void);
    AVCodecContext* alloc_decode_context(unsigned int stream);//alloc and return the codec context for the stream, caller must free it
    bool get_next_frame(AVPacket &packet);//writes the next frame out to packet, returns true on success, false on error (end of file)
    void unref_frame(AVPacket &packet);//free resources from frame
private:
    const std::string url;//the URL of the camera we are connecting to
    Config& cfg;



    AVFormatContext *input_format_context;
    AVPacket pkt;
};

#endif
