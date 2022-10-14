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
#This is the key we pull and preconfigure all VMs to accept
SSH_KEY_PATH=~/.ssh/id_rsa.pub
MAINTAINER_DIR=$(pwd)/$(dirname -- "${BASH_SOURCE[0]}")/..
OS_BASE_DIR=$MAINTAINER_DIR/os-images
#blank to skip signing
SIGN_KEY=chiton@edman007.com
PATH=$PATH:/sbin:/usr/sbin
#this file is the settings for the maintainer tools
#VMs will use ports staring here
BASE_PORT=10000
SSH_OFFSET=0
HTTP_OFFSET=1
HTTPS_OFFSET=2

#offset per VM
VM_OFFSET=5
HOST_GPG=$(command -v gpg2 || echo gpg)
if [ "$OS_TYPE" != "none" ]; then
    echo "Setting up $OS_TYPE/$OS_VERSION"
    #Settings for each VM kind
    #this script requires OS_TYPE and OS_VERSION is set
    if [ "$OS_TYPE" = "raspbian" ]; then
        TARGET_GPG=gpg
        if [ "$OS_VERSION" = "32" ]; then
            #raspbain32
            OS_NAME=raspbian-32
            OS_ID=0
            IMAGE_URL=https://downloads.raspberrypi.org/raspios_lite_armhf/images/raspios_lite_armhf-2022-04-07/2022-04-04-raspios-bullseye-armhf-lite.img.xz
        else
            #raspbain64
            OS_NAME=raspbian-64
            OS_ID=1
            IMAGE_URL=https://downloads.raspberrypi.org/raspios_lite_arm64/images/raspios_lite_arm64-2022-04-07/2022-04-04-raspios-bullseye-arm64-lite.img.xz
        fi
    elif [ "$OS_TYPE" = "debian" ]; then
        TARGET_GPG=gpg
        if [ "$OS_VERSION" = "11" ]; then
            #debian stable
            OS_NAME=debian-11
            OS_ID=2
            PACKAGE_URL=https://cdimage.debian.org/debian-cd/11.5.0/amd64/iso-cd/debian-11.5.0-amd64-netinst.iso
        elif [ "$OS_VERSION" = "testing" ]; then
            OS_NAME=debian-testing
            OS_ID=3
            #debian testing
            PACKAGE_URL=https://cdimage.debian.org/cdimage/daily-builds/daily/arch-latest/amd64/iso-cd/debian-testing-amd64-netinst.iso

        fi
    elif [ "$OS_TYPE" = "slackware" ]; then
        TARGET_GPG=gpg
        if [ "$OS_VERSION" = "15" ]; then
            #slackware 15
            OS_NAME=slackware-15
            OS_ID=4
            PACKAGE_URL=https://mirrors.slackware.com/slackware/slackware-iso/slackware64-15.0-iso/slackware64-15.0-install-dvd.iso
        elif [ "$OS_VERSION" = "current" ]; then
            OS_NAME=slackware-current
            OS_ID=5
            #slackware current
            PACKAGE_URL=https://mirrors.slackware.com/slackware/slackware64-current/usb-and-pxe-installers/usbboot.img

        fi

    else
        echo "Unsupported OS - $OS_TYPE - $OS_VERSION"
    fi

    echo "Setup for $OS_NAME"
    #Setup variables determined by this script
    OS_DIR=$OS_BASE_DIR/$OS_NAME
    BASE_PORT=`expr $BASE_PORT + $OS_ID \* $VM_OFFSET`
    SSH_PORT=`expr $BASE_PORT + $SSH_OFFSET`
    HTTP_PORT=`expr $BASE_PORT + $HTTP_OFFSET`
    HTTPS_PORT=`expr $BASE_PORT + $HTTPS_OFFSET`
    SSH_OPTS="-o UserKnownHostsFile=/dev/null -o StrictHostKeychecking=no -oPort=$SSH_PORT"
fi

if [ "x$CHITON_FUNCTIONS" = "x" ]; then
    CHITON_FUNCTIONS=1
    run_remote_cmd() {
        ssh $SSH_OPTS chiton-build@localhost "$1"
    }

    run_remote_script() {
        scp $SSH_OPTS "$1" chiton-build@localhost:run.sh
        run_remote_cmd 'chmod +x ./run.sh && ./run.sh'
    }


    wait_to_boot() {
        while true; do
            sleep 1
            if [ -r $OS_DIR/run.pid ]; then
                PID=$(cat $OS_DIR/run.pid)
                if ps -p $PID > /dev/null; then
                    if ssh $SSH_OPTS chiton-build@localhost 'echo Running' | grep Running ; then
                        return 0
                    fi
                fi
            fi
        done
        return 1
    }


    #CHITON_FUNCTIONS
fi
