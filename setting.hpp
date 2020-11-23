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
 *   Copyright 2020 Ed Martin <edman007@edman007.com>
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

const std::vector<Setting> setting_options {
    //DO NOT REMOVE THE COMMENT BELOW
    //PHP BELOW
 {"database-version", "0", "Database Version", "Updated to track the internal state of the database", SETTING_READ_ONLY},
 {"cfg-path", "", "Config File", "Path to the config file that is loaded", SETTING_READ_ONLY},
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
 {"timezone", "system", "Timezone", "The timezone to use, if set to \"system\" will use the system timezone)", SETTING_OPTIONAL_SYSTEM},
 {"video-url", "", "Video URL", "FFMPEG compatible URL for the camera", SETTING_REQUIRED_CAMERA},
 {"active", "0", "Camera Active", "set to 1 when the camera is active", SETTING_REQUIRED_CAMERA},
 {"camera-id", "", "Camera ID", "Used internally to track what is the active camera", SETTING_READ_ONLY},
 {"output-dir", "", "Output Directory", "The location to store videos", SETTING_REQUIRED_SYSTEM},
 {"ffmpeg-demux-options", "", "FFMPEG demux options", "Options for the ffmpeg demuxer", SETTING_OPTIONAL_CAMERA},
 {"reorder-queue-len", "0", "Reorder queue length", "How many packets to cache to properly resort frames "
         "(required for some cameras that give us out of order data even on TCP)", SETTING_OPTIONAL_CAMERA},
 {"seconds-per-file", "6", "Seconds per file", "How long a file should be, files are split at the next opprotunity after this, in seconds, Apple recommends 6", SETTING_OPTIONAL_SYSTEM},
 {"min-free-space", "1073741824", "min-free-space", "How many bytes of free space triggers a cleanup, if it contains a %, "
         "it is the target free-percentage of user accessable space", SETTING_OPTIONAL_SYSTEM},
 {"display-name", "", "Camera Name", "The name of the camera used in displays", SETTING_OPTIONAL_CAMERA},
 {"max-sync-offset", "5", "Max Sync Offset", "The maximum drift in camera time tolerated before we resync the clock", SETTING_OPTIONAL_CAMERA},
 {"socket-path", DEFAULT_SOCKET_PATH, "Control Socket Path", "The path for the control socket, required to manage the system from the web interface", SETTING_OPTIONAL_SYSTEM},
 {"broken-time-offset", "3600", "Broken Segment Future Offset", "Number of seconds in the future that will cause segments to be deleted", SETTING_OPTIONAL_SYSTEM},
};
