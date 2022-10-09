#!/bin/bash
cd `dirname $0`
set -x
VALID_VERSION=1
if [ "$1" != "32" ] &&  [ "$1" != "64" ] ; then
    VALID_VERSION=0
fi
if [ $# -lt 1 ] || [ "$VALID_VERSION" = "0" ]; then
    echo "Usage: $0 <version> [action]"
    echo -e "\tversion: 32 or 64"
    echo -e "\taction: (default is boot)"
    echo -e "\t\tclean - redownload the files"
    echo -e "\t\trebuild - reconfigure the OS image"
    echo -e "\t\tfreshen - reset the OS image to last configured"
    echo -e "\t\tboot - boot the OS"
    exit 1
fi

OS_TYPE=raspbian
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

if [ ! -f download.img ] || [ "$2" = "clean" ]; then
    rm -f download.img download.img.xz clean.img drive.img
    wget $IMAGE_URL -O download.img.xz
    echo 'Decompressing...'
    xz -d download.img.xz
fi

APPEND="root=/dev/mmcblk0p2 panic=1 rw rootwait dwc_otg.lpm_enable=0 dwc_otg.fiq_enable=0 dwc_otg.fiq_fsm_enable=0 dtoverlay=sdtweak,poll_once"
if [ "$OS_VERSION" = "32" ]; then
    CPU="-M raspi2b "
    QEMU_CMD=qemu-system-arm
else
    QEMU_CMD=qemu-system-aarch64
    CPU="-M raspi3b"
fi

if [ ! -f clean.img ] || [ "$2" = "rebuild" ]; then
    echo 'Creating fresh image'
    rm drive.img || true
    cp -v download.img clean.img
    echo 'Preconfiguring Image...'
    START=$(parted clean.img -m unit B  print 2>/dev/null |grep fat32|cut -d : -f 2|sed s/B//)
    touch ssh
    mcopy -i clean.img@@$START ssh ::
    rm ssh
    echo -n 'chiton-build:' > userconf
    #we delete this as soon as it boots
    echo 'raspberry' | openssl passwd -6 -stdin >> userconf
    mcopy -i clean.img@@$START userconf ::
    qemu-img resize -f raw clean.img 8G
    rm -f kernel system.dtb
    if [ "$OS_VERSION" = "32" ]; then
        mcopy -i clean.img@@$START ::kernel7l.img kernel #kernel, kernel7, kernel7l, kernel8
        mcopy -i clean.img@@$START ::bcm2709-rpi-2-b.dtb system.dtb #bcm2709-rpi-2-b.dtb, bcm2710-rpi-2-b.dtb
    else
        mcopy -i clean.img@@$START ::kernel8.img kernel #kernel, kernel7, kernel7l, kernel8
        mcopy -i clean.img@@$START ::bcm2710-rpi-3-b.dtb system.dtb #bcm2709-rpi-2-b.dtb, bcm2710-rpi-2-b.dtb
    fi
    #boot it
    #the dwg_otg stuff is because the RPI USB driver sucks and somehow QEMU makes it pop up even more
    #https://www.osadl.org/Single-View.111+M5c03315dc57.0.html
    $QEMU_CMD \
        $CPU \
        -append "$APPEND" \
        -dtb $OS_DIR/system.dtb -kernel $OS_DIR/kernel -no-reboot \
        -drive if=sd,file=$OS_DIR/clean.img,format=raw \
        -device usb-net,netdev=$OS_NAME -netdev user,id=$OS_NAME,hostfwd=tcp::$SSH_PORT-:22,hostfwd=tcp::$HTTP_PORT-:80 \
        -pidfile $OS_DIR/run.pid &
    QEMU_JOB=$!

    #generate the preconfigure script and run it
    SSH_KEY_TXT=$(cat $SSH_KEY_PATH)
    cat <<EOF > preconf.sh
#!/bin/bash
set -x
PATH=$PATH:/sbin:/usr/sbin:/bin:/usr/bin
cmd_log () {
  tee /home/chiton-build/preconf.log
}
mkdir -p ~/.ssh/
echo '$SSH_KEY_TXT' > ~/.ssh/authorized_keys
chmod 700 ~/.ssh/
chmod 600 ~/.ssh/authorized_keys
#sudo passwd -d chiton-build 2>&1 | cmd_log
#insecure method for debugging
echo 'chiton-build:EW1kpmTAt4WWDwRyjazS' | sudo chpasswd

#based on the resize script in /usr/lib/raspi-config/initrd_resize.sh
ROOT_PART_DEV=\$(sudo findmnt / -o source -n)
ROOT_PART_NAME=\$(echo "\$ROOT_PART_DEV" | cut -d "/" -f 3)
ROOT_DEV_NAME=\$(echo /sys/block/*/"\${ROOT_PART_NAME}" | cut -d "/" -f 4)
ROOT_DEV="/dev/\${ROOT_DEV_NAME}"
ROOT_PART_NUM=\$(sudo cat "/sys/block/\${ROOT_DEV_NAME}/\${ROOT_PART_NAME}/partition")
ROOT_DEV_SIZE=\$(sudo cat "/sys/block/\${ROOT_DEV_NAME}/size")
TARGET_END=\$((ROOT_DEV_SIZE - 1))

sudo parted -m "\$ROOT_DEV" u s resizepart "\$ROOT_PART_NUM" "\$TARGET_END" 2>&1 | cmd_log
sudo resize2fs "\$ROOT_PART_DEV" 2>&1 | cmd_log
df -h 2>&1 | cmd_log
echo 'Acquire::Retries "3";' | sudo tee /etc/apt/apt.conf.d/80-retries 2>&1 | cmd_log
echo 'Updating OS'
#mandb makes the system bog way down, don't need it updating
sudo rm -fv /var/lib/man-db/auto-update  2>&1 | cmd_log || true
echo 'man-db	man-db/auto-update	boolean	false' | sudo debconf-set-selections
sudo sed -i 's/CONF_SWAPSIZE=100/CONF_SWAPSIZE=1024/' /etc/dphys-swapfile 2>&1 | cmd_log
sudo systemctl stop dphys-swapfile 2>&1 | cmd_log
sudo systemctl start dphys-swapfile 2>&1 | cmd_log
sudo apt-get update -y 2>&1 | cmd_log
sudo apt-get upgrade -y 2>&1 | cmd_log
sudo apt-mark hold raspberrypi-kernel 2>&1 | cmd_log
echo 'Preseed Complete!'
EOF
cat preconf.sh
    cat <<EOF > ssh-pass.sh
#!/usr/bin/env expect
set timeout -1
log_user 1
set cmd [lrange \$argv 0 end]
set ret 255
eval spawn \$cmd
expect {
  "password:" {
    send "raspberry\n";
    send_user "raspberry\n";
    set ret 0
  }
  "reset by peer" {
    exit 1
  }
  "refused" {
    exit 2
  }
}
expect eof
send_user "Got EOF"
wait
send_user "Command exited"
exit \$ret
EOF

    chmod +x ./ssh-pass.sh
    echo 'Waiting to boot'
    while ! ./ssh-pass.sh scp $SSH_OPTS preconf.sh chiton-build@localhost:  2>&1 > /dev/null; do
        echo -n .
        sleep 10;
    done
    echo '\nConfiguring...'
    while ! ./ssh-pass.sh ssh $SSH_OPTS chiton-build@localhost 'chmod +x preconf.sh ; ./preconf.sh' 2>&1  ; do
        echo -n .
        sleep 10
    done
    echo 'Updating Kernel...'
    if [ "$OS_VERSION" = "32" ]; then
        scp $SSH_OPTS chiton-build@localhost:/boot/kernel7l.img kernel
        scp $SSH_OPTS chiton-build@localhost:/boot/bcm2709-rpi-2-b.dtb system.dtb
    else
        scp $SSH_OPTS chiton-build@localhost:/boot/kernel8.img kernel
        scp $SSH_OPTS chiton-build@localhost:/boot/bcm2710-rpi-3-b.dtb system.dtb
    fi

    echo -e '\nComplete, shutting down..'
    run_remote_cmd 'sudo systemctl poweroff' || true
    wait $QEMU_JOB

    #cleanup
    rm ssh-pass.sh preconf.sh
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

$QEMU_CMD \
    $CPU \
    -append "$APPEND" \
    -dtb $OS_DIR/system.dtb -kernel $OS_DIR/kernel -no-reboot \
    -drive if=sd,file=$OS_DIR/drive.img,format=raw \
    -device usb-net,netdev=$OS_NAME -netdev user,id=$OS_NAME,hostfwd=tcp::$SSH_PORT-:22,hostfwd=tcp::$HTTP_PORT-:80 \
    -pidfile $OS_DIR/run.pid  -daemonize

wait_to_boot


echo 'Booted'
