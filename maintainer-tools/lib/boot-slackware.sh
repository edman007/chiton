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
set -x
if [ $# -lt 1 ]; then
    echo "Usage: $0 <version> [action]"
    echo -e "\tversion: 15, current"
    echo -e "\taction: (default is boot)"
    echo -e "\t\tclean - redownload the files"
    echo -e "\t\trebuild - reconfigure the OS image"
    echo -e "\t\tfreshen - reset the OS image to last configured"
    echo -e "\t\tboot - boot the OS"
    exit 1
fi

OS_TYPE=slackware
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
    rm -f pkg.iso clean.img drive.img || true
    wget $PACKAGE_URL -O pkg.iso
fi

if [ ! -f clean.img ] || [ "$2" = "rebuild" ]; then
    echo 'Creating fresh image'
    rm -f initrd.preseed initrd.preseed.lzma initrd.xz || true
    rm -f etc/rc.d/rc.S || true
    rmdir etc/rc.d || true
    rmdir etc/ || true

    #unpack the initrd
    cp -v $MAINTAINER_DIR/lib/slackware-preseed.sh preseed.sh
    cp $SSH_KEY_PATH id_rsa.pub
    isoinfo -i pkg.iso -x '/ISOLINUX/INITRD.IMG' > initrd.xz
    isoinfo -i pkg.iso -x '/KERNELS/HUGE.S/BZIMAGE' > kernel
    xz -dc < initrd.xz > initrd.preseed
    cpio -i -d -F initrd.preeseed etc/rc.d/rc.S
    sed -i '/BOGUS_LOGIN/d' etc/rc.d/rc.S
    echo '/preseed.sh' >> etc/rc.d/rc.S
    echo 'reboot' >> etc/rc.d/rc.S
    chmod +x preseed.sh
    echo preseed.sh | cpio -H newc -o -A -F initrd.preseed
    echo id_rsa.pub | cpio -H newc -o -A -F initrd.preseed
    echo etc/rc.d/rc.S | cpio -H newc -o -A -F initrd.preseed
    xz -z -F lzma initrd.preseed
    rm -f etc/rc.d/rc.S
    rmdir etc/rc.d
    rmdir etc/
    rm -f drive.img clean.img || true

    qemu-img create -f raw $OS_DIR/clean.img 8G

    #boot it
    qemu-system-x86_64 \
        -machine accel=kvm \
        -drive format=raw,file=$OS_DIR/clean.img,discard=unmap \
        -m 6G \
        -smp 4 -no-reboot \
        -device e1000,netdev=$OS_NAME -netdev user,id=$OS_NAME,hostfwd=tcp::$SSH_PORT-:22,hostfwd=tcp::$HTTP_PORT-:80 \
        -pidfile $OS_DIR/run.pid \
        -append 'kbd=1 load_ramdisk=1 prompt_ramdisk=0 rw printk.time=0 nomodeset SLACK_KERNEL=huge.s' \
        -initrd $OS_DIR/initrd.preseed.lzma -kernel $OS_DIR/kernel \
        -cdrom $OS_DIR/pkg.iso
    #it just exits on completion
    rm -f initrd.preseed initrd.preseed.lzma initrd.xz kernel || true
    rm -f id_rsa.pub preseed.sh
    exit 1
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
