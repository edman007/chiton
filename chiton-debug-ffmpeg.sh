#!/bin/bash
#This is my development script to test a debug environment
#some people might find this helpful
set -e
#ssh queenofhearts -L 5550:192.168.2.50:554 -L 5551:192.168.2.51:554 -L 5552:192.168.2.52:554 -L 5553:192.168.2.53:554 -L 5554:192.168.2.54:554 -L 5555:192.168.2.55:554
#to use custom ffmpeg
export FFMPEG_LIBS="-L/home/edman007/tmp/FFmpeg/libavcodec/ -L/home/edman007/tmp/FFmpeg/libavdevice -L/home/edman007/tmp/FFmpeg/libavfilter -L/home/edman007/tmp/FFmpeg/libavformat -L/home/edman007/tmp/FFmpeg/libavresample -L/home/edman007/tmp/FFmpeg/libavutil -L/home/edman007/tmp/FFmpeg/libpostproc -L/home/edman007/tmp/FFmpeg/libswresample -L/home/edman007/tmp/FFmpeg/libswscale -lavformat -lavcodec -lavutil -lavfilter"
export FFMPEG_CFLAGS="-I/home/edman007/tmp/FFmpeg/"
export LD_LIBRARY_PATH="/home/edman007/tmp/FFmpeg/libavcodec/:/home/edman007/tmp/FFmpeg/libavdevice:/home/edman007/tmp/FFmpeg/libavfilter:/home/edman007/tmp/FFmpeg/libavformat:/home/edman007/tmp/FFmpeg/libavresample:/home/edman007/tmp/FFmpeg/libavutil:/home/edman007/tmp/FFmpeg/libpostproc:/home/edman007/tmp/FFmpeg/libswresample:/home/edman007/tmp/FFmpeg/libswscale:$LD_LIBRARY_PATH"
#make clean
#./configure --enable-debug
make -j15
trap "kill -- -$$" EXIT
#fix RadeonHD bug with VAAPI by running glxgears without vblank to keep the GPU awake
vblank_mode=0 glxgears &
(sleep 5 ; chmod 777 chiton.sock) &
#gdb ./chiton -ex 'break image_util.cpp:53'  -ex 'r -c config/chiton.cfg -d'
#gdb ./chiton -ex 'break chiton_ffmpeg.cpp:1007' -ex 'r -c config/chiton.cfg -d'
#gdb ./chiton  -ex 'r -c config/chiton.cfg -d'
#valgrind ./chiton -c config/chiton.cfg -d
./chiton -c config/chiton.cfg -d
