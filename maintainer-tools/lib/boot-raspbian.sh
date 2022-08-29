#!/bin/bash
cd `dirname $0`

if [ $# -lt 1 ]; then
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
            wait $PID
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

APPEND="'root=/dev/mmcblk0p2 panic=1 rw rootwait dwc_otg.lpm_enable=0 dwc_otg.fiq_enable=0 dwc_otg.fiq_fsm_enable=0'"
if [ "$OS_VERSION" = "32" ]; then
    CPU="-M raspi2b "
    QEMU_CMD=qemu-system-arm
else
    QEMU_CMD=qemu-system-aarch64
    CPU="-M raspi3b"
fi

if [ ! -f clean.img ] || [ "$2" = "rebuild" ]; then
    echo 'Creating fresh image'
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
        -append 'root=/dev/mmcblk0p2 panic=1 rw rootwait dwc_otg.lpm_enable=0 dwc_otg.fiq_enable=0 dwc_otg.fiq_fsm_enable=0' \
        -dtb $OS_DIR/system.dtb -kernel $OS_DIR/kernel -no-reboot \
        -drive if=sd,file=$OS_DIR/clean.img,format=raw -serial stdio \
        -device usb-net,netdev=$OS_NAME -netdev user,id=$OS_NAME,hostfwd=tcp::$SSH_PORT-:22,hostfwd=tcp::$HTTP_PORT-:80 \
        -pidfile $OS_DIR/run.pid &
    QEMU_JOB=$!

    #generate the preconfigure script and run it
    SSH_KEY_TXT=$(cat $SSH_KEY_PATH)
    cat <<EOF > preconf.sh
#!/bin/bash
PATH=$PATH:/sbin:/usr/sbin:/bin:/usr/bin
mkdir -p ~/.ssh/
echo '$SSH_KEY_TXT' > ~/.ssh/authorized_keys
chmod 700 ~/.ssh/
chmod 600 ~/.ssh/authorized_keys
sudo passwd -d chiton-build
sudo apt update -y
sudo shutdown -h now
EOF

    cat <<EOF > ssh-pass.sh
#!/usr/bin/env expect
set timeout 20

set cmd [lrange \$argv 0 end]
set ret 255
eval spawn \$cmd
expect {
  "password:" {
    send "raspberry\n";
    set ret 0
  }
  "reset by peer" {
    exit 1
  }
  "refused" {
    exit 2
  }
}
wait
exit \$ret
EOF

    chmod +x ./ssh-pass.sh
    echo -n 'Waiting to boot'
    while ! ./ssh-pass.sh scp $SSH_OPTS preconf.sh chiton-build@localhost:  2>&1 > /dev/null; do
        echo -n .
        sleep 10;
    done
    echo -ne '\nConfiguring...'
    while ! ./ssh-pass.sh ssh $SSH_OPTS chiton-build@localhost 'chmod +x preconf.sh ; ./preconf.sh'  2>&1 > /dev/null ; do
        echo -n .
        sleep 10
    done

    #cleanup
    rm ssh-pass.sh preconf.sh

    echo -e '\nComplete, shutting down..'
    wait $QEMU_JOB
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
    -append $APPEND \
    -dtb $OS_DIR/system.dtb -kernel $OS_DIR/kernel -no-reboot \
    -drive if=sd,file=$OS_DIR/drive.img,format=raw -serial stdio \
    -device usb-net,netdev=$OS_NAME -netdev user,id=$OS_NAME,hostfwd=tcp::$SSH_PORT-:22,hostfwd=tcp::$HTTP_PORT-:80 \
    -pidfile $OS_DIR/run.pid &

wait_to_boot


echo 'Booted'
