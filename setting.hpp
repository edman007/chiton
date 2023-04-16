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
 *   Copyright 2020-2022 Ed Martin <edman007@edman007.com>
 *
 **************************************************************************
 */

#include <vector>
#include "config_build.hpp"

/*
 * This file is included by config.cpp to provide default values
 *
 * This file is also converted to php, as such it must contain just the config
 * definitions and no C++ code other than that below the "PHP BELOW" comment
 *
 */

// priorties need to be manually copied into web/inc/configdb.php.in

enum SettingPriority {
    SETTING_READ_ONLY,//Not settable via the database, cannot be set to anything meaningful after starting, may contain useful information
    SETTING_REQUIRED_SYSTEM,//system requirement, that is always used and does not have a sane default
    SETTING_OPTIONAL_SYSTEM,//optional system requirement
    SETTING_REQUIRED_CAMERA,//required to be set on all cameras
    SETTING_OPTIONAL_CAMERA,//optional extra feature for cameras

};

/*
 * Defines the format of settings, changes here will drive updates to web_config.php
 */
struct Setting {
    std::string key;//the key name
    std::string def;//default
    std::string short_description;//human readable name
    std::string description;//full length description
    SettingPriority priority;//priority
};

//comments starting with @key become long documentation
const std::vector<Setting> setting_options {
    //DO NOT REMOVE THE COMMENT BELOW
    //PHP BELOW
 /* Ignore Comment to fix AWK script
  */
 {"database-version", "0", "Database Version", "Updated to track the internal state of the database", SETTING_READ_ONLY},
  /* @database-version Internal use only
  */
 {"cfg-path", "", "Config File", "Path to the config file that is loaded", SETTING_READ_ONLY},
  /* @cfg-path Only has effect when set via the command line.
  * This is passed on the command line to set the location of the configuration file
  */
 {"pid-file", "", "PID File", "path to write the PID to", SETTING_READ_ONLY},
 {"fork", "0", "Fork", "Set to non-zero to fork to the background", SETTING_READ_ONLY},
 {"privs-user", "", "System User", "System Username to drop privs to (the daemon runs as this user)", SETTING_READ_ONLY},
 {"db-host", "", "Database Host", "The hostname of the database to connect to", SETTING_READ_ONLY},
 {"db-user", "", "Database User", "The username to use when connecting to the database", SETTING_READ_ONLY},
 {"db-password", "", "Database Password", "The password to use when connecting to the database", SETTING_READ_ONLY},
 {"db-database", "", "Database Name", "The name of the database on the database server to use", SETTING_READ_ONLY},
 {"db-socket", "", "Database Socket", "Socket path to socket for DB server", SETTING_READ_ONLY},
 {"db-port", "", "Database Port", "Port to connect to (if not local) of the database", SETTING_READ_ONLY},
 {"verbosity", "3", "Verbosity", "Logging verbosity (higher is more verbose, lower is less verbose, range 0-5)", SETTING_OPTIONAL_SYSTEM},
 {"log-history-len", "50", "Log History Length <0+>", "Length of the message log (for each camera)", SETTING_OPTIONAL_SYSTEM},
 {"timezone", "system", "Timezone", "The timezone to use, if set to \"system\" will use the system timezone)", SETTING_OPTIONAL_SYSTEM},
 {"video-url", "", "Video URL", "FFMPEG compatible URL for the camera", SETTING_REQUIRED_CAMERA},
 {"active", "0", "Camera Active", "set to 1 when the camera is active", SETTING_REQUIRED_CAMERA},
 {"camera-id", "", "Camera ID", "Used internally to track what is the active camera", SETTING_READ_ONLY},
 {"output-dir", "", "Output Directory", "The location to store videos", SETTING_REQUIRED_SYSTEM},
  /* @output-dir Eventually this is planned to work on a per-camera basis, however this has not been implemented yet,
  * setting it for a specific camera may cause undefined behavior
  */
 {"ffmpeg-demux-options", "", "FFMPEG demux options", "Options for the ffmpeg demuxer", SETTING_OPTIONAL_CAMERA},
  /* @ffmpeg-demux-options These are option passed to libavformat when demuxing (reading a stream), see man ffmpeg-formats
  */
 {"ffmpeg-mux-options", "", "FFMPEG mux options", "Options for the ffmpeg muxer", SETTING_OPTIONAL_CAMERA},
  /* @ffmpeg-mux-options These are option passed to libavformat when muxing (writing a stream), see man ffmpeg-formats
  */
 {"reorder-queue-len", "0", "Reorder queue length", "How many packets to cache to properly resort frames "
         "(required for some cameras that give us out of order data even on TCP)", SETTING_OPTIONAL_CAMERA},
  /* @reorder-queue-len Specifically implemented for reolink cameras that transmit packets out of order, this will reorder those parkets and correct
   * some types of stream corruption
  */
 {"interleave-queue-depth", "4", "Interleaving Queue Depth", "How many packets to look when writing to verify the duration is always correct. "
  "Should be large enough to ensure the queue contains at least 2 of every packet type", SETTING_OPTIONAL_CAMERA},
  /* @reorder-queue-len Specifically implemented for reolink cameras that transmit packets out of order, this will reorder those parkets and correct
   * some types of stream corruption
  */
 {"seconds-per-file", "360", "Seconds per file", "How long a file should be, files are split at the next opprotunity after this, in seconds", SETTING_OPTIONAL_SYSTEM},
 /* @seconds-per-file when using fMP4, this is the length of the individual mp4 files and it is also the unit size that is deleted when removing old videos
  */
 {"seconds-per-segment", "6", "Seconds per segment", "How long a segment should be, files are segmented at the next opprotunity after this, in seconds, Apple recommends 6", SETTING_OPTIONAL_SYSTEM},
 /* @seconds-per-segment Smaller values can improve latency and give finer control of when skipping the video, however it produces more records in the database
  * and results in larger .m3u8 files to the browser. Smaller values can also allow you to view closer to realtime video in the web player. Setting this to a value less than the period
  * between keyframes results in the period between keyframes being the controlling factor.
  */
 {"min-free-space", "1073741824", "Miniumn Free Space", "How many bytes of free space triggers a cleanup, if it contains a %, "
         "it is the target free-percentage of user accessable space", SETTING_OPTIONAL_SYSTEM},
 /* @min-free-space Needs to be at least enough to accomidate 10 seconds of recording
  */
 {"display-name", "", "Camera Name", "The name of the camera used in displays", SETTING_OPTIONAL_CAMERA},
 {"max-sync-offset", "5", "Max Sync Offset", "The maximum drift in camera time tolerated before we resync the clock", SETTING_OPTIONAL_CAMERA},
 {"socket-path", DEFAULT_SOCKET_PATH, "Control Socket Path", "The path for the control socket, required to manage the system from the web interface", SETTING_OPTIONAL_SYSTEM},
 {"broken-time-offset", "3600", "Broken Segment Future Offset", "Number of seconds in the future that will cause segments to be deleted", SETTING_OPTIONAL_SYSTEM},
 {"encode-format-video", "copy", "Video Encoding codec <copy|h264|hevc>", "The codec to save video with, copy will use the camera compressed video if compatible", SETTING_OPTIONAL_CAMERA},
 {"encode-format-audio", "copy", "Audio Encoding codec <copy|none|aac|ac3>", "The codec to save video with, copy will use the camera compressed video if compatible", SETTING_OPTIONAL_CAMERA},
 {"ffmpeg-encode-audio-opt", "", "FFMPEG audio encode options", "Options for the ffmpeg encoder - audio", SETTING_OPTIONAL_CAMERA},
 {"ffmpeg-encode-video-opt", "", "FFMPEG video encode options", "Options for the ffmpeg encoder - video", SETTING_OPTIONAL_CAMERA},
 {"ffmpeg-decode-audio-opt", "", "FFMPEG audio decode options", "Options for the ffmpeg decoder - audio", SETTING_OPTIONAL_CAMERA},
 {"ffmpeg-decode-video-opt", "", "FFMPEG video decode options", "Options for the ffmpeg decoder - video", SETTING_OPTIONAL_CAMERA},
 {"audio-bitrate", "auto", "audio bitrate", "Bitrate, in kbps, to encode the audio at (or 'auto' to autoselect)", SETTING_OPTIONAL_CAMERA},
 {"video-bitrate", "auto", "video bitrate", "Bitrate, in kbps, to encode the video at (or 'auto' to autoselect)", SETTING_OPTIONAL_CAMERA},
 {"output-extension", ".mp4", "Output Extension <.ts|.mp4>", "HLS Output Format, .ts for mpeg-ts files, .mp4 for fragmented mp4", SETTING_OPTIONAL_CAMERA},
 {"video-encode-method", "auto", "Video Encode method <auto|sw|vaapi|v4l2>", "Method to use for encoding, vaapi and v4l2 are the supported HW encoders,"
  " auto tries both HW decoders before defaulting to SW, SW is a fallback", SETTING_OPTIONAL_CAMERA},
 {"video-decode-method", "auto", "Video Decode method <auto|sw|vaapi|vdpau|v4l2>", "Method to use for decoding, vaapi, vdpau, and v4l2 are supported HW decoders,"
  " auto tries both HW decoders before defaulting to SW, SW is a fallback", SETTING_OPTIONAL_CAMERA},
 {"video-hw-pix-fmt", "auto", "Video pixel format <auto|[fmt]>", "Pixel format to use, auto or any valid ffmpeg format is accepted,"
  " but not all will work with your HW", SETTING_OPTIONAL_CAMERA},
 {"log-color-fatal", "5", "Color for CLI FATAL error Messages <1..255>", "ANSI color code for use on CLI", SETTING_OPTIONAL_SYSTEM},
 {"log-color-error", "1", "Color for CLI ERROR error Messages <1..255>", "ANSI color code for use on CLI", SETTING_OPTIONAL_SYSTEM},
 {"log-color-warn", "3", "Color for CLI WARN error Messages <1..255>", "ANSI color code for use on CLI", SETTING_OPTIONAL_SYSTEM},
 {"log-color-info", "6", "Color for CLI INFO error Messages <1..255>", "ANSI color code for use on CLI", SETTING_OPTIONAL_SYSTEM},
 {"log-color-debug", "2", "Color for CLI DEBUG error Messages <1..255>", "ANSI color code for use on CLI", SETTING_OPTIONAL_SYSTEM},
 {"log-color-enabled", "1", "Enable CLI color <1|0>", "Enable Color Logging", SETTING_OPTIONAL_SYSTEM},
 {"log-name-length", "16", " Logging Camera Name Length", "Maxiumn Length of thread name (typically the camera) to copy into log messages", SETTING_OPTIONAL_SYSTEM},
 {"motion-mods", "cvdetect", "Motion Algorithms <none|abc[,xyz...]>", "List of motion algorithms to run against, ',' delimited", SETTING_OPTIONAL_CAMERA},
 /* @motion-mods Current modules are: cvbackground (computes an average background), cvdebugshow (if compiled with debug support, shows the frames being processed),
    cvdetect (performs motion detection), cvmask (subtracts the background to identify changed areas), opencv (converts frames into opencv for processing)
  */
 {"event-mods", "db,console", "Event Notification Algorithms <none|abc[,xyz...]>", "List of Event Notification algorithms to run against, ',' delimited", SETTING_OPTIONAL_CAMERA},
 /* @event-mods Current modules are db (writes events to the databse) and console (prints events to logs)
  */
 {"motion-cvmask-tau", "0.01", "Motion CVMask Tau <0-1>", "Tau parameter for CVMask Algorithm", SETTING_OPTIONAL_CAMERA},
 {"motion-cvmask-beta", "15.0", "Motion CVMask Tau <0.0-255.0>", "Beta parameter for CVMask Algorithm", SETTING_OPTIONAL_CAMERA},
 {"motion-cvmask-threshold", "20", "Motion CVMask Threshold <1-255>", "Threshold parameter for CVMask Algorithm", SETTING_OPTIONAL_CAMERA},
 {"motion-cvmask-delay", "30", "Motion CVMask Threshold <1-600>", "Frame Delay Count before applying sensitivity mask", SETTING_OPTIONAL_CAMERA},
 {"motion-cvdetect-dist", "150.0", "Motion CVDetect Miniumn Distance <0-10000>", "Miniumn distance between objects to count as two objects", SETTING_OPTIONAL_CAMERA},
 {"motion-cvdetect-area", "1000.0", "Motion CVDetect Miniumn Area <0->", "Miniumn area of an object to send motion events", SETTING_OPTIONAL_CAMERA},
 {"motion-cvdetect-tracking", "10", "Motion CVDetect Tracking Time <0->", "Number of frames that an object must be tracked to trigger an event", SETTING_OPTIONAL_CAMERA},
 {"motion-cvbackground-tau", "0.001", "Motion CVBackground Tau <0-1>", "Tau parameter for CVBackground Algorithm", SETTING_OPTIONAL_CAMERA},
 {"motion-opencv-map-indirect", "true", "Motion OpenCV Indirect Mapping <true|false>", "Use libva to map the image and copy the brightness, "
  "recommended for AMD GPUs", SETTING_OPTIONAL_CAMERA},
 {"motion-opencv-map-cl", "true", "Motion OpenCV OpenCL Mapping <true|false>", "Use ffmpeg to map it to opencl, and map that buffer directly to opencv", SETTING_OPTIONAL_CAMERA},
 {"motion-opencv-map-vaapi", "true", "Motion OpenCV's VA-API Mapping <true|false>", "Use OpenCV to map the direct VA-API buffer", SETTING_OPTIONAL_CAMERA},
 {"motioncontroller-skip-ratio", "0.05", "Motion Controller Skip Ratio <0-1>", "The ratio of time that should be spent blocking waiting for packets, 0 disables motion controller "
  "skipping, 1 will cause all frames to skip motion processing", SETTING_OPTIONAL_CAMERA},
 {"video-encode-b-frame-max", "16", "Video Encode Maximum B-frames <0->", "Maximum B-frames when encoding", SETTING_OPTIONAL_CAMERA},
 /* @video-encode-b-frame-max When encoding video, this is the maxium b-frames that the encoder should put out between keyframes
  * Some encoders, such as the V4L2 encoder on the raspberry pi do not suppor this and it must be set to 0.
  * Lower values generally reduce compression, higher values can cause seconds-per-segment to be exceeded
  */
 {"motion-cvresize-scale", "1", "Motion Detection Scale ratio <0-1>", "Scale ratio for motion processing, 1 is no scaling, 0.5 is downscale 50%", SETTING_OPTIONAL_CAMERA},
 {"ffmpeg-log-level", "40", "FFMpeg log level <-8-56>", "FFMpeg Log level, ffmpeg messages above this will not be printed", SETTING_OPTIONAL_SYSTEM},
 /* @opencv-disable-opencl Useful to set to true when you have a buggy/broken OpenCL implementation. Note: Setting to "false"
  * does not re-enable it even with a full system reload, application must be fully restarted to re-enable it.
  */
 {"opencv-disable-opencl", "false", "Disable OpenCL Acceleration in OpenCV <true|false|>", "Set to true to disable OpenCL use by OpenCV "
  "(forces opencv motion detection to SW)", SETTING_OPTIONAL_SYSTEM},
};
