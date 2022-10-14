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


cd `dirname $0`

if [ $# -lt 1 ]; then
    echo "Usage: $0 <version> [action]"
    echo -e "\tversion: 11, 12, or testing"
    echo -e "\taction: (default is boot)"
    echo -e "\t\tclean - redownload the files"
    echo -e "\t\trebuild - reconfigure the OS image"
    echo -e "\t\tfreshen - reset the OS image to last configured"
    echo -e "\t\tboot - boot the OS"
    exit 1
fi

OS_TYPE=debian
OS_VERSION="$1"

. settings.sh

mkdir -p $OS_DIR
cd $OS_DIR

#Don't wait for it to shutdown if we are rebuilding the image, just kill it
if [ "$2" = "clean" ] || [ "$2" = "rebuild" ] || [ "$2" = "freshen" ]; then
    if [ -r $OS_DIR/run.pid ]; then
        PID=$(cat $OS_DIR/run.pid)
        if ps -p $PID > /dev/null; then
            echo 'Killing running instance'
            kill $PID
            sleep 1
            rm -f $OS_DIR/run.pid
        fi
    fi
fi

if [ ! -f pkg.iso ] || [ "$2" = "clean" ]; then
    rm -f pkg.iso clean.img drive.img initrd.gz vmlinuz || true
    wget $PACKAGE_URL -O pkg.iso
fi

if [ ! -f clean.img ] || [ "$2" = "rebuild" ]; then
    echo 'Creating fresh image'
    cp -v $MAINTAINER_DIR/lib/deb-preseed.cfg preseed.cfg
    cp $SSH_KEY_PATH id_rsa.pub
    cp $MAINTAINER_DIR/lib/install-setup.sh .
    isoinfo -i pkg.iso -x '/INSTALL.AMD/INITRD.GZ;1' > initrd.gz
    isoinfo -i pkg.iso -x '/INSTALL.AMD/VMLINUZ.;1' > vmlinuz
    gzip -dc initrd.gz > initrd.preseed
    echo preseed.cfg | cpio -H newc -o -A -F initrd.preseed
    echo id_rsa.pub | cpio -H newc -o -A -F initrd.preseed
    chmod +x install-setup.sh
    echo install-setup.sh | cpio -H newc -o -A -F initrd.preseed
    gzip -1f initrd.preseed
    rm -f drive.img clean.img
    qemu-img create -f raw $OS_DIR/clean.img 8G

    #boot it
    qemu-system-x86_64 \
        -machine accel=kvm \
        -drive format=raw,file=$OS_DIR/clean.img,discard=unmap \
        -m 6G \
        -smp 12 \
        -device e1000,netdev=$OS_NAME -netdev user,id=$OS_NAME,hostfwd=tcp::$SSH_PORT-:22,hostfwd=tcp::$HTTP_PORT-:80 \
        -pidfile $OS_DIR/run.pid \
        -initrd $OS_DIR/initrd.preseed.gz -kernel $OS_DIR/vmlinuz \
        -cdrom $OS_DIR/pkg.iso
    #it just exits on completion
fi

if [ ! -f drive.img ] || [ "$2" = "freshen" ]; then
    echo 'Writing fresh image'
    cp -v clean.img drive.img
fi

#check if qemu is running
if [ -r $OS_DIR/run.pid ]; then
    PID=$(cat $OS_DIR/run.pid)
    if ps -p $PID > /dev/null; then
        if wait_to_boot ; then
            echo 'QEMU is Running'
            exit 0
        fi
    fi
fi

qemu-system-x86_64 \
    -machine accel=kvm \
    -drive format=raw,file=$OS_DIR/drive.img,discard=unmap \
    -m 6G \
    -smp 12 \
    -device e1000,netdev=$OS_NAME -netdev user,id=$OS_NAME,hostfwd=tcp::$SSH_PORT-:22,hostfwd=tcp::$HTTP_PORT-:80 \
    -pidfile $OS_DIR/run.pid -daemonize

wait_to_boot
echo 'Booted'
