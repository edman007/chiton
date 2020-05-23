#ifndef __CHITON_FFMPEG_HPP__
#define __CHITON_FFMPEG_HPP__

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
};

//fix FFMPEG and c++1x issues
#ifdef av_err2str
#undef av_err2str
av_always_inline char* av_err2str(int errnum)
{
    static char str[AV_ERROR_MAX_STRING_SIZE];
    memset(str, 0, sizeof(str));
    return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
}
#endif


//make AVRounding valid in bitwise operations
inline AVRounding operator|(AVRounding a, AVRounding b)
{
    return static_cast<AVRounding>(static_cast<int>(a) | static_cast<int>(b));
}

void load_ffmpeg(void);

#endif
