#!/bin/bash
 ##########################################################################
 #
 #     This file is part of Chiton.
 #
 #   Chiton is free software: you can redistribute it and/or modify
 #   it under the terms of the GNU General Public License as published by
 #   the Free Software Foundation, either version 3 of the License, or
 #   (at your option) any later version.
 #
 #   Chiton is distributed in the hope that it will be useful,
 #   but WITHOUT ANY WARRANTY; without even the implied warranty of
 #   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 #   GNU General Public License for more details.
 #
 #   You should have received a copy of the GNU General Public License
 #   along with Chiton.  If not, see <https://www.gnu.org/licenses/>.
 #
 #   Copyright 2022 Ed Martin <edman007@edman007.com>
 #
 ##########################################################################
set -e
cd install-test
#answer questions
#echo "chiton	chiton/dbconfig-remove	boolean	true" | sudo debconf-set-selections



if dpkg -l | grep chiton ; then
    sudo DEBIAN_FRONTEND=noninteractive apt-get purge -y chiton || true
    sudo DEBIAN_FRONTEND=noninteractive apt-get purge -y chiton-dbgsym || true
fi
#install curl, needed for our scripts
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y curl
#install mariadb
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y mariadb-server
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y ./chiton_*.deb

if curl -s http://localhost/chiton/ | grep Error ; then
    echo 'Error, Does not appear to work after first install'
    exit 1
fi

#test if it's actually the chiton webpage responding
if ! curl -s http://localhost/chiton/ | grep 'static/chiton.js' ; then
    echo 'Error, http://localhost/chiton/ does not look correct'
    exit 1
fi

#get database login
#DB_USER=$(sudo cat /etc/chiton/chiton.cfg | grep db-user | sed -e s/\'//g -e 's/ //g'  | cut -d = -f 2)
#DB_PASS=$(sudo cat /etc/chiton/chiton.cfg | grep db-password | sed -e s/\'//g -e 's/ //g'  | cut -d = -f 2)

#setup a server with the config
curl -s -d create_camera=1 -X POST http://localhost/chiton/settings.php | grep -A 1 statusmsg
curl -s -d 'name[0]=video-url&value[0]=file:///home/chiton-build/install-test/vids/front-1440p-h264-aac.mpg&camera[0]=0' \
     -X POST 'http://localhost/chiton/settings.php?camera=0' | grep -A 1 statusmsg
curl -s -d 'name[0]=active&value[0]=1&camera[0]=0' \
     -X POST 'http://localhost/chiton/settings.php?camera=0' | grep -A 1 statusmsg
curl -s -d reload_backend=1 -X POST http://localhost/chiton/settings.php | grep -A 1 statusmsg

#record stuff for 60 seconds
echo 'Waiting for video to be processed'
sleep 60
echo "Stopping Record"
#and shutdown
curl -s -d 'name[0]=active&value[0]=0&camera[0]=0' \
     -X POST 'http://localhost/chiton/settings.php?camera=0' | grep -A 1 statusmsg
curl -s -d reload_backend=1 -X POST http://localhost/chiton/settings.php | grep -A 1 statusmsg

#verify video written
echo "Checking files exist"
CAM_ACTIVE=$(sudo ls /var/lib/chiton/vids/ | wc -l)
if [[ "$CAM_ACTIVE" != 1 ]]; then
    echo "Failed Camera Count Test"
    exit 1
fi

echo "Checking Stream info"
#verify that the webserver kept it
STREAM_URL=$(curl -s -L  'http://localhost/chiton/stream.php?stream=-1&id=0' | tail -n1 )
STREAM_CNT=$(curl -s -L  "http://localhost/chiton/$STREAM_URL" | wc -l )

if [[ "$STREAM_CNT" -lt "30" ]]; then
    echo "Stream packet count unreasonably low, $STREAM_CNT"
    exit 1
fi

echo "Checking access to file"
#check that the metadata is right for the stream and that we can access it
LAST_VID_CHUNK=$(curl -s -L  "http://localhost/chiton/$STREAM_URL" | tail -n1 )
LAST_VID_LEN=$(curl -s -L  "http://localhost/chiton/$STREAM_URL" | tail -n2 |head -n1|cut -d @ -f 2 )
if ! curl -s -I "http://localhost/chiton/$LAST_VID_CHUNK" | grep '200 OK' ; then
    echo "Cannot access vid chunk $LAST_VID_CHUNK"
    exit 1;
fi

echo "checking file size"
FILE_LEN=$(curl -s -I "http://localhost/chiton/$LAST_VID_CHUNK" | grep 'Content-Length'|cut -d : -f 2 |sed 's/ //')
if [[ "$FILE_LEN" -lt "$LAST_VID_LEN" ]]; then
    echo "End of video chunk appears invalid";
    exit 1
fi

echo "All Good!"
