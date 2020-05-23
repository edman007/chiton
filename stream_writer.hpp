#ifndef __STREAM_WRITER_HPP__
#define __STREAM_WRITER_HPP__
#include "chiton_config.hpp"
#include "stream_unwrap.hpp"

class StreamWriter {
public:
    StreamWriter(Config cfg, std::string path, StreamUnwrap &unwrap) : cfg(cfg), path(path), unwrap(unwrap) {};
    ~StreamWriter();
    
    bool  open();//open the file for writing, returns true on success
    void close(void);
    bool write(const AVPacket &pkt);//write the packet to the file
private:
    Config &cfg;
    std::string path;
    StreamUnwrap &unwrap;
    
    AVFormatContext *output_format_context = NULL;
    int stream_mapping_size = 0;
    int *stream_mapping = NULL;
    
};
#endif
