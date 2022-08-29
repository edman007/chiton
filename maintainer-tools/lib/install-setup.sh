#!/bin/sh

#Preload the ssh key of the current user
if [ -r /id_rsa.pub ]; then
    mkdir -p  /target/home/chiton-build/.ssh/
    cp /id_rsa.pub /target/home/chiton-build/.ssh/authorized_keys
    mkdir -p  /target/root/.ssh/
    cp /id_rsa.pub /target/root/.ssh/authorized_keys

    #run in a chroot so it sees the /etc/passwd
    chroot /target chown -R chiton-build:chiton-build /home/chiton-build/.ssh/
    chmod 700 /target/home/chiton-build/.ssh/
    chmod 600 /target/home/chiton-build/.ssh/authorized_keys
    chown -R root:root /target/root/.ssh/
    chmod 700 /target/root/.ssh/
    chmod 600 /target/root/.ssh/authorized_keys
fi

#No Password sudo
echo -e "\n#chiton-build is god\nchiton-build ALL=(ALL) NOPASSWD: ALL" >> /target/etc/sudoers
