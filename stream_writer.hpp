#ifndef __STREAM_WRITER_HPP__
#define __STREAM_WRITER_HPP__
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
 **************************************************************************
 */
#include "chiton_config.hpp"
#include "stream_unwrap.hpp"
#include <functional>


class PacketInterleavingBuf {
public:
    AVPacket* in;//the origional packet, in source timebase
    AVPacket* out;//the packet to be written, in destination timebase
    bool video_keyframe;
    long dts0;//DTS in stream 0 timebase
    PacketInterleavingBuf(const AVPacket *pkt);//clone in & out to pkt
    ~PacketInterleavingBuf();//free the packets
};

class StreamWriter {
public:
    StreamWriter(Config &cfg, std::string path);
    StreamWriter(Config &cfg);
    ~StreamWriter();
    
    bool open(void);//open the file for writing, returns true on success
    long long close(void);//close the file and return the filesize
    bool write(const AVPacket *pkt, const AVStream *in_stream);//write the packet to the file, interleave the packet appropriatly
    bool write(const AVFrame *frame, const AVStream *in_stream);//write the frame to the file (using encoding)
    //set the path of the file to write to, if the stream was opened, it remains opened (and a new file is created), if it was closed then it is not opened
    long long change_path(const std::string &new_path = "");//returns file position of the end of the file
    bool add_stream(const AVStream *in_stream);//add in_stream to output (copy)

    bool add_encoded_stream(const AVStream *in_stream, const AVCodecContext *dec_ctx, const AVFrame *frame);//add in_stream to output, specify frame to pass a HW context
    bool add_encoded_stream(const AVStream *in_stream, const AVCodecContext *dec_ctx);//add in_stream to output
    bool copy_streams(StreamUnwrap &unwrap);//copy all streams from unwrap to output
    bool is_fragmented(void) const;//return true if this is a file format that supports fragmenting
    long long get_init_len(void) const;//return the init length, -1 if not valid
    void set_keyframe_callback(std::function<void(const AVPacket *pkt, StreamWriter &out)> cbk);//set a callback when writing video keyframes
    std::string get_codec_str(void) const;//returns the codec string for the current output
    bool have_video(void) const ;//true if video is included in output
    bool have_audio(void) const;//true if audio is included in output
    int get_width() const;//returns the video width, or -1 if unknown
    int get_height() const;//returns the video height, or -1 if unknown
    float get_framerate() const;//returns the video framerate, or 0 if unknown
    //return true if we found our encoding parameters, writes out pixel format expected by the encoder, and codec_id and profile as we use
    bool get_video_format(const AVFrame *frame, AVPixelFormat &pix_fmt, AVCodecID &codec_id, int &codec_profile) const;
    unsigned int get_bytes_written(void);//return the number of bytes written since last call to get_bytes_written()
private:
    Config &cfg;
    Config *cfg1;
    Config *cfg2;
    std::string path;
    enum {SEGMENT_MPGTS, SEGMENT_FMP4} segment_mode;
    
    AVFormatContext *output_format_context = NULL;
    std::map<int,int> stream_mapping;
    std::map<int, AVCodecContext*> encode_ctx;
    std::map<int, std::string> codec_str;
    //metadata that we track to forward to the HLS frontend (needs to go into the DB)
    bool video_stream_found;
    bool audio_stream_found;
    int video_width;//-1 if unknown
    int video_height;//-1 if unknown
    float video_framerate;//0 if unknown
    //these offsets are used to shift the time when receiving something
    std::vector<long> stream_offset;
    std::vector<long> last_dts;//used to fix non-increasing DTS
    std::function<void(const AVPacket *pkt, StreamWriter &out)> keyframe_cbk;//we call this on all video keyframe packets

    //used to track if we were successful in getting a file opened (and skip writing anything if not successful)
    //true means the file is opened
    bool file_opened;

    //buffers used for FMP4 mode to split the file
    uint8_t *init_seg;
    long long init_len;
    AVIOContext *output_file;

    AVPacket *enc_pkt;//buffer used when writing files

    //buffers used to force proper interleaving
    std::list<PacketInterleavingBuf*> interleaving_buf;
    std::map<int,long> interleaved_dts;

    unsigned int write_counter;//used to provide write stats

    void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt, const std::string &tag);
    bool open_path(void);//open the path and write the header
    void free_context(void);//free the context associated with a file
    bool alloc_context(void);
    AVStream *init_stream(const AVStream *in_stream);//Initilizes output stream
    long long frag_stream(void);//creates a fragment at the current location and returns the position
    long long write_buf(bool reopen);//write the buffer to the file, return bytes written on success, negative on error, if reopen is true then a new buffer is also allocated
    long long write_init(void);//write the init segment to the buffer
    bool write(PacketInterleavingBuf *buf);//write the packet to the file (perform the actual write)
    void interleave(PacketInterleavingBuf *buf);//add the packet to the interleaving buffer and write buffers that are ready
    bool write_interleaved(void);//write any buffers that are ready to be written
    bool gen_codec_str(const int stream, const AVCodecParameters *codec);//generate the HLS codec ID for the stream
    uint8_t *nal_unit_extract_rbsp(const uint8_t *src, const uint32_t src_len, uint32_t *dst_len);//helper for decoding h264 types
    bool guess_framerate(const AVStream *stream);//guess the framerate from the stream, true if found framerate
    bool guess_framerate(const AVCodecContext* codec_ctx);//guess the framerate from the codec context, true if found framerate
    const AVCodec* get_vencoder(int width, int height, AVCodecID &codec_id, int &codec_profile) const;//return the encoder, or null if we can't encode this, write out the codec id/profile
    const AVCodec* get_vencoder(int width, int height) const;//return the encoder, or null if we can't encode this
    bool validate_codec_tag(AVStream *stream);//check and modifiy the codec tag to ensure it is compatible with the output format
};
#endif
