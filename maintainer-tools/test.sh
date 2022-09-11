#!/bin/bash
set -e
cd `dirname $0`
OS_TYPE=none
. ./lib/settings.sh
show_help () {
    echo -e "$0 <arg> [-a]\n"
    echo -e "\thelp - show this help"
    echo -e "\tbuild - build golden packages (x84-64)"
    echo -e "\ttest - build and test golden packages (x84-64)"
    exit 0
}
VERSION=unknown
GPG_NEW_PASS=x

#gets the version number
get_version () {
    VERSION=$(grep PACKAGE_VERSION config_build.hpp | cut -d ' ' -f 3 | tr -d '"')
}

#Creates the official source tarball in release/
build_source () {
    get_version
    #only if not done yet
    if [ ! -f ../release/chiton-$VERSION.tar.gz ]; then
    (
        #build the golden packages
        cd ..
        make maintainer-clean
        ./configure
        make -j15 release
        if [ "x$SIGN_KEY" != "x" ]; then
            cd release
            $HOST_GPG  --homedir $OS_BASE_DIR/gpg/home --command-fd 0  --status-fd 1 --pinentry-mode loopback --armor --default-key $SIGN_KEY --clearsign *.dsc <<< $GPG_NEW_PASS
            mv *.dsc.asc *.dsc
            for i in *.[xg]z ; do
	            $HOST_GPG  --homedir $OS_BASE_DIR/gpg/home  --command-fd 0  --status-fd 1 --pinentry-mode loopback --default-key  $SIGN_KEY  -b  $i <<< $GPG_NEW_PASS
            done
        fi

    )
    fi
}

#write the private key to a variable, SIGN_KEY_UNLOCK
gpg_unlock () {
    if [ "x$SIGN_KEY" != "x" ];  then
        rm -rf $OS_BASE_DIR/gpg/ || true
        mkdir -p $OS_BASE_DIR/gpg/home
        chmod 700 $OS_BASE_DIR/gpg/home $OS_BASE_DIR/gpg/
        touch $OS_BASE_DIR/gpg/pass-test
        #now we prompt for password
        echo '================================================='
        echo '== Enter the password for your GPG key:        =='
        echo "== $SIGN_KEY =="
        GPG_NEW_PASS=$(dd if=/dev/random bs=1 count=15 2>/dev/null  | mimencode)
        while true ; do
            echo -n 'Password: '
            read -s GPG_PASS
            ret=0
            $HOST_GPG --no-use-agent --command-fd 0  --status-fd 1 --pinentry-mode loopback --default-key $SIGN_KEY -s $OS_BASE_DIR/gpg/pass-test \
                 <<< $GPG_PASS | grep SIG_CREATED  || ret=$?
            if [ $ret = 0 ] ; then
                break;
            fi
        done
        rm $OS_BASE_DIR/gpg/pass-test $OS_BASE_DIR/gpg/pass-test.gpg || true
        $HOST_GPG --command-fd 0  --status-fd 2 --pinentry-mode loopback --export-secret-key -a "$SIGN_KEY" > $OS_BASE_DIR/gpg/priv.key <<< $GPG_PASS
        $HOST_GPG --homedir $OS_BASE_DIR/gpg/home --command-fd 0  --status-fd 1 --pinentry-mode loopback --import $OS_BASE_DIR/gpg/priv.key <<< $GPG_PASS
        rm $OS_BASE_DIR/gpg/priv.key
        GPG_NEW_PASS=$(dd if=/dev/random bs=1 count=15 2>/dev/null  | mimencode)
        $HOST_GPG --homedir $OS_BASE_DIR/gpg/home  --command-fd 0  --status-fd 1 --pinentry-mode loopback --passwd $SIGN_KEY \
             <<< $GPG_PASS$'\n'$GPG_NEW_PASS
    fi
}

push_signing_key () {
    if [ "x$SIGN_KEY" != "x" ];  then
        REMOTE_KEYID=$(run_remote_cmd "$TARGET_GPG --with-colons  --list-key $SIGN_KEY | grep ^fpr: | head -1 | cut -d : -f 10")
        if [ "x$REMOTE_KEYID" != "x" ]; then
            run_remote_cmd "$TARGET_GPG --batch  --no-tty --yes --delete-secret-and-public-key $REMOTE_KEYID"
        fi
        $HOST_GPG --homedir $OS_BASE_DIR/gpg/home --command-fd 0  --status-fd 1 --pinentry-mode loopback --export-secret-key -a "$SIGN_KEY" <<< $GPG_NEW_PASS \
            | run_remote_cmd "$TARGET_GPG --import --no-tty --batch --command-fd 0"
    fi

}

pull_build_files () {
    scp $SSH_OPTS chiton-build@localhost:pkg/*.{deb,build,changes,buildinfo} chiton-build@localhost:pkg/chiton_*.dsc ../release
}
#on debian based systems, builds the .deb, needs OS version info
run_deb_build () {
    run_remote_cmd 'rm -rf /home/chiton-build/pkg/ ; mkdir -p /home/chiton-build/pkg/'
    scp $SSH_OPTS ../release/*.dsc ../release/*.tar.gz chiton-build@localhost:pkg/
    run_remote_script ./lib/build-deb-pkg.sh <<< $SIGN_KEY$'\n'$GPG_NEW_PASS
}

build_arm () {
    ./lib/boot-raspbian.sh 32 freshen
    ./lib/boot-raspbian.sh 64 freshen
}


build_debian () {
    #./lib/boot-deb.sh 11 rebuild
    #./lib/boot-deb.sh 11 freshen
    ./lib/boot-deb.sh 11 boot
    #./lib/boot-deb.sh testing freshen
}

#must be called with OS_TYPE & OS_VERSION set and setting.sh already called
build_deb_pkg () {
    push_signing_key
    run_deb_build
    pull_build_files
}

test_install () {
    echo 'test install'
    gpg_unlock
    #build_source
    #build_debian
    OS_TYPE=debian
    OS_VERSION=11
    . ./lib/settings.sh
    build_deb_pkg
}


if [ $# != 1 ]; then
    show_help
fi

if [ "$1" == "help" ]; then
    show_help
fi

if [ "$1" == "build" ]; then
    build_arm
fi

if [ "$1" == "test" ]; then
    test_install
fi
