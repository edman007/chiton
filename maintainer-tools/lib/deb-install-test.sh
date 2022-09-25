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
set -x
cd install-test
#answer questions


if dpkg -l | grep chiton ; then
    echo "chiton	chiton/dbconfig-remove	boolean	true" | sudo debconf-set-selections
    sudo DEBIAN_FRONTEND=noninteractive apt-get purge -y chiton || true
    sudo DEBIAN_FRONTEND=noninteractive apt-get purge -y chiton-dbgsym || true
fi
#install curl, needed for our scripts
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y curl
#install mariadb
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y mariadb-server
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y ./chiton_*.deb

function pre_configure_check () {
    if curl -s http://localhost/chiton/ | grep Error ; then
        echo 'Error, Does not appear to work after first install'
        exit 1
    fi

    #test if it's actually the chiton webpage responding
    if ! curl -s http://localhost/chiton/ | grep 'static/chiton.js' ; then
        echo 'Error, http://localhost/chiton/ does not look correct'
        exit 1
    fi
}

#get database login
function configure () {
    #setup a server with the config
    curl -s -d create_camera=1 -X POST http://localhost/chiton/settings.php | grep -A 1 statusmsg
    curl -s -d 'name[0]=video-url&value[0]=/home/chiton-build/install-test/vids/front-1440p-h264-aac.mpg&camera[0]=0' \
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

}

function post_configure_check () {
    sudo apachectl configtest
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
    if [[ "${LAST_VID_CHUNK:0:1}" = '#' ]]; then
        echo 'Video files appear missing!'
        exit 1
    fi
    LAST_VID_LEN=$(curl -s -L  "http://localhost/chiton/$STREAM_URL" | tail -n2 |head -n1|cut -d @ -f 2 )
    if ! curl -s -I "http://localhost/chiton/$LAST_VID_CHUNK" | grep '200 OK' ; then
        echo "Cannot access vid chunk $LAST_VID_CHUNK"
        exit 1;
    fi

    echo "checking file size"
    FILE_LEN=$(curl -s -I "http://localhost/chiton/$LAST_VID_CHUNK" | grep 'Content-Length'|cut -d : -f 2 |tr -d ' ' | tr -d '\r' )
    if [ "$FILE_LEN" -lt "$LAST_VID_LEN" ]; then
        echo "End of video chunk appears invalid";
        exit 1
    fi
}

function deconfigure_check (){
    if [[ -d /var/lib/chiton/ ]]; then
        echo "/var/lib/chiton/ exists";
        exit 1
    fi
    if [[ -d /var/run/chiton/ ]]; then
        echo "/var/run/chiton/ exists";
        exit 1
    fi
    if [[ -d /etc/chiton/ ]]; then
        echo "/etc/chiton/ exists";
        exit 1
    fi
    sudo apachectl configtest
    if ! sudo mysqldump chiton > /dev/null ; then
        echo 'DB not removed'
        exit 1
    fi
}


function upgrade_test() {
    sudo systemctl stop chiton
    #load the DB with a very old config
    DB_USER=$(sudo cat /etc/chiton/chiton.cfg | grep db-user | sed -e s/\'//g -e 's/ //g'  | cut -d = -f 2)
    DB_PASS=$(sudo cat /etc/chiton/chiton.cfg | grep db-password | sed -e s/\'//g -e 's/ //g'  | cut -d = -f 2)
    echo "SELECT concat('DROP TABLE IF EXISTS \`', table_name, '\`;') FROM information_schema.tables WHERE table_schema = 'chiton';" | \
        mysql -u$DB_USER -p"$DB_PASS" | tail -n +2 > db_update.sql
    cat /home/chiton-build/install-test/vids/0.1-default.sql >> db_update.sql
    mysql -u$DB_USER -p"$DB_PASS" chiton < db_update.sql
    sudo systemctl start chiton
    #verify that the config has a new version
    sleep 10
    sudo systemctl stop chiton
    UPDATED_DB_VERSION=$(echo "SELECT value FROM config WHERE name = 'database-version' AND camera = -1;" |     mysql -u$DB_USER -p"$DB_PASS" chiton | tail -n1)
    echo "DB Version is $UPDATED_DB_VERSION"
    if [[ "$UPDATED_DB_VERSION" = "1.0" ]]; then
        echo 'Failed'
        exit 1
    fi
}

pre_configure_check
configure
post_configure_check

#remove and reinstall
echo "chiton chiton/dbconfig-remove	boolean	false" | sudo debconf-set-selections
sudo DEBIAN_FRONTEND=noninteractive apt-get remove chiton -y
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y ./chiton_*.deb
echo 'Checking reinstalled'
post_configure_check
echo "chiton chiton/dbconfig-remove	boolean	true" | sudo debconf-set-selections
sudo DEBIAN_FRONTEND=noninteractive apt-get purge chiton -y
deconfigure_check
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y ./chiton_*.deb
upgrade_test

echo "All Good!"
