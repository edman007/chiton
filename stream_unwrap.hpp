#ifndef __STREAM_UNWRAP_HPP__
#define __STREAM_UNWRAP_HPP__

#include "chiton_config.hpp"

class StreamUnwrap {
    /*
     * Handles the Network half of the video
     *
     */
public:
    StreamUnwrap(Config& cfg);
    bool connect(void);//returns true on success
private:
    const std::string url;//the URL of the camera we are connecting to
    Config& cfg;

    bool open_output(void);
    
    void load_avformat(void);


};

#endif
