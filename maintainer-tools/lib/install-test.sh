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

if which dpkg 2> /dev/null ; then
    ON_DEBIAN=1
else
    ON_DEBIAN=0
fi

if [[ $ON_DEBIAN = 1 ]] ; then
    if dpkg -l | grep chiton ; then
        echo "chiton	chiton/dbconfig-remove	boolean	true" | sudo debconf-set-selections || true
        echo "chiton	chiton/purge	boolean	true" | sudo debconf-set-selections || true
        sudo DEBIAN_FRONTEND=noninteractive apt-get purge -y chiton || true
        sudo DEBIAN_FRONTEND=noninteractive apt-get purge -y chiton-dbgsym || true
    fi
    #update the os
    sudo DEBIAN_FRONTEND=noninteractive apt-get update -y
    sudo DEBIAN_FRONTEND=noninteractive apt-get upgrade -y
    #install curl and expect, needed for our scripts
    sudo DEBIAN_FRONTEND=noninteractive apt-get install -y curl expect
    #install mariadb
    sudo DEBIAN_FRONTEND=noninteractive apt-get install -y mariadb-server
    sudo DEBIAN_FRONTEND=noninteractive apt-get install -y ./chiton_*.deb
else
    export PATH=/sbin:/usr/sbin:/bin:/usr/bin
    sudo /etc/rc.d/rc.chiton stop || true
    sudo removepkg chiton || true
    LASTUPDATE=0
    if [ -r /var/lib/slackpkg/LASTUPDATE ]; then
        LASTUPDATE=$(cat /var/lib/slackpkg/LASTUPDATE)
    fi
    FREQ=$(expr 60 \* 60 \* 24)
    CUR_TIME=$(date +%s)
    UPDATE_DIFF=$(expr $CUR_TIME - $LASTUPDATE - $FREQ)
    if [[ $UPDATE_DIFF > 0 ]]; then
        sudo slackpkg -batch=on -default_answer=y update
        sudo slackpkg -batch=on -default_answer=y upgrade-all || true
        sudo lilo
    fi

    #and install
    sudo installpkg ./chiton*.txz
    sudo chiton-install -y
    sudo chmod +x /etc/rc.d/rc.chiton
    sudo /etc/rc.d/rc.chiton start
    sudo /etc/rc.d/rc.httpd restart

fi

#/home/chiton-build may be non-readable by the daemon, so lets put vids/ into tmp/
rm -rf /tmp/vids || true
cp -r /home/chiton-build/install-test/vids/ /tmp/vids
chmod 755 /tmp/vids
chmod 644 /tmp/vids/*

function start_chiton () {
    if [ $ON_DEBIAN = 1 ] ; then
        sudo systemctl start chiton
    else
        sudo /etc/rc.d/rc.chiton start
    fi

}

function stop_chiton () {
    if [ $ON_DEBIAN = 1 ] ; then
        sudo systemctl stop chiton
    else
        sudo /etc/rc.d/rc.chiton stop
    fi
}
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
    #wait for the socket to be listening
    SOCK_CLOSED=1
    for time in {0..60}; do
        if sudo fuser /var/run/chiton/chiton.sock ; then
            SOCK_CLOSED=0
            break
        fi
        sleep 1
    done
    if [ 1 = $SOCK_CLOSED ] ; then
        echo 'Socket not open'
        exit 1
    fi

    #should be done within 10 minutes
    TIMEOUT=600
    SUPERSLOW=0
    if uname -m | grep -E '(arm|aarch64)' ; then
        #arm is really slow...so give it an hour to finish
        TIMEOUT=3600
        SUPERSLOW=1
    fi
    cat > expect_chiton_log <<EOF
#!/usr/bin/env expect
set timeout $TIMEOUT
log_user 1
set cmd [lrange \$argv 0 end]
eval spawn \$cmd
expect {
  "Lost connection to cam 0" {
    send_user "Image Processed\n"
    close
    exit 0
  }
}
send_user "Timeout!\n"
exit 255
EOF
    chmod +x expect_chiton_log

    #set the verbosity way up
    echo -en "\nverbosity=5" | sudo tee -a /etc/chiton/chiton.cfg

    #setup a server with the config
    curl -s -d create_camera=1 -X POST http://localhost/chiton/settings.php | grep -A 1 statusmsg
    curl -s -d 'name[0]=video-url&value[0]=/tmp/vids/front-1440p-h264-aac.mpg&camera[0]=0' \
         -X POST 'http://localhost/chiton/settings.php?camera=0' | grep -A 1 statusmsg
    curl -s -d 'name[0]=active&value[0]=1&camera[0]=0' \
         -X POST 'http://localhost/chiton/settings.php?camera=0' | grep -A 1 statusmsg
    curl -s -d 'name[0]=motioncontroller-skip-ratio&value[0]=0&camera[0]=0' \
         -X POST 'http://localhost/chiton/settings.php?camera=0' | grep -A 1 statusmsg
    if [ $SUPERSLOW = 1 ]; then
        #disable motion detection on RPI, it's too slow
        curl -s -d 'name[0]=motion-mods&value[0]=none&camera[0]=0' \
             -X POST 'http://localhost/chiton/settings.php?camera=0' | grep -A 1 statusmsg
    fi
    curl -s -d reload_backend=1 -X POST http://localhost/chiton/settings.php | grep -A 1 statusmsg

    sleep 10
    #check that the stats page is working
    curl -s 'http://localhost/chiton/stats.php' | grep -A 1 'Loading camera 0'
    curl -s 'http://localhost/chiton/stats.php' | grep -A 1 '[RUNNING]'

    #record stuff for 60 seconds
    echo 'Waiting for video to be processed'
    if [ $ON_DEBIAN = 1 ] ; then
        sudo ./expect_chiton_log journalctl -u chiton -f --no-pager -n0
    else
        sudo ./expect_chiton_log tail -f /var/log/syslog -n0
    fi
    echo "Stopping Record"
    #and shutdown
    curl -s -d 'name[0]=active&value[0]=0&camera[0]=0' \
         -X POST 'http://localhost/chiton/settings.php?camera=0' | grep -A 1 statusmsg
    curl -s -d reload_backend=1 -X POST http://localhost/chiton/settings.php | grep -A 1 statusmsg

    EVCOUNT=$(curl 'http://localhost/chiton/settings.php?camera=0' | wc -l)
    if [ $SUPERSLOW = 0 ] && [ $EVCOUNT -lt 10 ]; then
        echo 'Event count is too low'
        exit 1
    fi

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

    if [[ "$STREAM_CNT" -lt "76" ]]; then
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

    echo "Checking Locking"
    STARTTIME=$(curl -L  'http://localhost/chiton/info.php?id=0' | grep --color -E -o 'starttime":([0-9]+)'|cut -d : -f 2)
    FIRST_GAP=$(curl -L  'http://localhost/chiton/info.php?id=0' | grep --color -E -o '"len":([0-9]+)'|cut -d : -f 2|head -1)
    TZERO_SEC=$(expr $FIRST_GAP + $STARTTIME)
    THIRTY_SEC=$(expr $TZERO_SEC + 30)
    FORTYFIVE_SEC=$(expr $TZERO_SEC + 45)
    NINTEY_SEC=$(expr $TZERO_SEC + 90)

    #compute start time
    for start_end in $TZERO_SEC:$THIRTY_SEC $FORTYFIVE_SEC:$NINTEY_SEC ; do
        START=$(echo $start_end | cut -d : -f 1)
        END=$(echo $start_end | cut -d : -f 2)
        echo "S: $START, E: $END"
        START_DAY=$STARTTIME
        START_HOUR=$(date --date=@$START +%k)
        START_MIN=$(date --date=@$START +%M)
        START_SEC=$(date --date=@$START +%S)

        END_DAY=$STARTTIME
        END_HOUR=$(date --date=@$END +%k)
        END_MIN=$(date --date=@$END +%M)
        END_SEC=$(date --date=@$END +%S)
        echo "from_d=$START_DAY&from_h=$START_HOUR&from_m=$START_MIN&from_s=$START_SEC&to_d=$END_DAY&to_h=$END_HOUR&to_m=$END_MIN&to_s=$END_SEC"
        curl -s -d "from_d=$START_DAY&from_h=$START_HOUR&from_m=$START_MIN&from_s=$START_SEC&to_d=$END_DAY&to_h=$END_HOUR&to_m=$END_MIN&to_s=$END_SEC" \
             -X POST 'http://localhost/chiton/camera.php?id=0'  | grep -A 1 statusmsg | grep Locked
        echo "export=1&start_day_ts=$START_DAY&start_h=$START_HOUR&start_m=$START_MIN&start_s=$START_SEC&end_day_ts=$END_DAY&end_h=$END_HOUR&end_m=$END_MIN&end_s=$END_SEC"
        curl -s -d "export=1&start_day_ts=$START_DAY&start_h=$START_HOUR&start_m=$START_MIN&start_s=$START_SEC&end_day_ts=$END_DAY&end_h=$END_HOUR&end_m=$END_MIN&end_s=$END_SEC" \
             -X POST 'http://localhost/chiton/camera.php?id=0'  | grep -A 1 statusmsg | grep 'Job Started'

    done

    #now wait for them to show up
    EXPORT_COMPLETE=0
    for t in {0..120}; do
        sleep 1
        COMPLETE=$(curl -L  'http://localhost/chiton/camera.php?id=0' | grep download | wc -l)
        if [[ "x$COMPLETE" = "x2" ]]; then
            EXPORT_COMPLETE=1
            break;
        fi
    done

    if [[ "$EXPORT_COMPLETE" = 0 ]]; then
        echo 'Export Failed'
        exit 1
    fi

    for t in {0..1}; do
        EXPORT_ID=$(curl -L  'http://localhost/chiton/camera.php?id=0' | grep export_id | grep -o -E 'value="[0-9]+' | cut -d \" -f 2 | head -1)
        curl -s -d "delete_export=1&export_id=$EXPORT_ID" \
             -X POST 'http://localhost/chiton/camera.php?id=0'  | grep -A 1 statusmsg | grep 'Job Deleted'
    done

    EXPORT_COMPLETE=$(curl -L  'http://localhost/chiton/camera.php?id=0' | grep download | wc -l)
    if [[ "x$EXPORT_COMPLETE" != "x0" ]]; then
        echo 'Export show after deleting both'
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
    if sudo mysqldump chiton > /dev/null ; then
        echo 'DB not removed'
        exit 1
    fi
}


function upgrade_test() {
    stop_chiton
    #load the DB with a very old config
    DB_USER=$(sudo cat /etc/chiton/chiton.cfg | grep db-user | sed -e s/\'//g -e 's/ //g'  | cut -d = -f 2)
    DB_PASS=$(sudo cat /etc/chiton/chiton.cfg | grep db-password | sed -e s/\'//g -e 's/ //g'  | cut -d = -f 2)
    echo "SELECT concat('DROP TABLE IF EXISTS \`', table_name, '\`;') FROM information_schema.tables WHERE table_schema = 'chiton';" | \
        mysql -u$DB_USER -p"$DB_PASS" | tail -n +2 > db_update.sql
    cat /home/chiton-build/install-test/vids/0.1-default.sql >> db_update.sql
    mysql -u$DB_USER -p"$DB_PASS" chiton < db_update.sql
    start_chiton
    #verify that the config has a new version
    sleep 10
    stop_chiton
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

#reinstall
if [ $ON_DEBIAN = 1 ] ; then
    echo "chiton chiton/dbconfig-remove	boolean	false" | sudo debconf-set-selections
    echo "chiton	chiton/purge	boolean	false" | sudo debconf-set-selections
    sudo DEBIAN_FRONTEND=noninteractive apt-get remove chiton -y
    sudo DEBIAN_FRONTEND=noninteractive apt-get install -y ./chiton_*.deb
else
    sudo upgradepkg --reinstall ./chiton*.txz
    sudo /etc/rc.d/rc.chiton restart
fi
echo 'Checking reinstalled'
post_configure_check

stop_chiton
#remove
if [ $ON_DEBIAN = 1 ] ; then
    echo "chiton chiton/dbconfig-remove	boolean	true" | sudo debconf-set-selections
    echo "chiton	chiton/purge	boolean	true" | sudo debconf-set-selections
    sudo DEBIAN_FRONTEND=noninteractive apt-get purge chiton -y
else
    sudo chiton-install -Gy
    sudo removepkg chiton
fi


deconfigure_check

#install again
if [ $ON_DEBIAN = 1 ] ; then
    sudo DEBIAN_FRONTEND=noninteractive apt-get install -y ./chiton_*.deb
else
    sudo installpkg ./chiton*.txz
    sudo chiton-install -y
    sudo chmod +x /etc/rc.d/rc.chiton
    sudo /etc/rc.d/rc.httpd restart
fi
start_chiton
upgrade_test
start_chiton
configure
post_configure_check
stop_chiton
echo "All Good!"
