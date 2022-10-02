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
cd `dirname $0`
OS_TYPE=none
. ./lib/settings.sh
show_help () {
    echo -e "$0 <arg> [addational args]\n"
    echo -e "\thelp - show this help and exit"
    echo -e "\tclean - Erase VMs and exit"
    echo -e "\timage - Build & Image VMs"
    echo -e "\tboot - Startup VMs"
    echo -e "\tsource - Package Source"
    echo -e "\tbuild - Build packages"
    echo -e "\ttest - Run tests"
    echo -e "\tgold - Build and test golden packages (includes full OS rebuilds)"
    echo -e "\t-a - Run desired command for all supported systems"
    echo -e "\t-s - Build Source"
    echo -e "\t-r - Rebuild Binary"
    exit 0
}
VERSION=unknown
GPG_NEW_PASS=x


RUN_IMAGE=0
RUN_BOOT=0
RUN_SOURCE=0
RUN_BUILD=0
RUN_TEST=0

ALL_HOSTS=0

#gets the version number
get_version () {
    VERSION=$(grep PACKAGE_VERSION ../config_build.hpp | cut -d ' ' -f 3 | tr -d '"')
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
    get_version
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
    rm -rf ../release/$OS_NAME || true
    mkdir -p ../release/$OS_NAME
    scp $SSH_OPTS chiton-build@localhost:pkg/*.{deb,build,changes,buildinfo} chiton-build@localhost:pkg/chiton_*.dsc ../release/$OS_NAME/
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


#must be called with OS_TYPE & OS_VERSION set and setting.sh already called
build_deb_pkg () {
    push_signing_key
    run_deb_build
    pull_build_files
}

test_install () {
    if ! [[ -d $OS_BASE_DIR/sample-videos ]] ; then
        #we may want to move this, but for now it's our official sample
        mkdir $OS_BASE_DIR/sample-videos/
        curl --output $OS_BASE_DIR/sample-videos/front-1440p-h264-aac.mpg https://dev.edman007.com/~edman007/pub/chiton-dev/front-1440p-h264-aac.mpg
        curl --output $OS_BASE_DIR/sample-videos/0.1-default.sql https://dev.edman007.com/~edman007/pub/chiton-dev/0.1-default.sql
    fi

    if [ "$OS_TYPE" = "debian" ] || [ "$OS_TYPE" = "raspbian" ]; then
        run_remote_cmd 'rm -rf install-test ; mkdir -p install-test/vids'
        scp $SSH_OPTS ../release/$OS_NAME/*.deb chiton-build@localhost:install-test
        scp $SSH_OPTS $OS_BASE_DIR/sample-videos/* chiton-build@localhost:install-test/vids
        run_remote_script ./lib/deb-install-test.sh

    fi
}



if [ $# = 0 ]; then
    show_help
    exit 0
fi

for arg in "$@"
do
    case $arg in

        "help")
            show_help
            exit 0
            ;;
        "-h")
            show_help
            exit 0
            ;;
        "clean")
            rm -rf $OS_BASE_DIR
            exit 0
            ;;
        "image")
            RUN_IMAGE=1
            ;;
        "boot")
            RUN_BOOT=1
            ;;
        "build")
            RUN_BUILD=1
            ;;
        "test")
            RUN_TEST=1
            ;;
        "gold")
            ALL_HOSTS=1
            RUN_TEST=1
            rm -rf $OS_BASE_DIR || true
            rm -rf ../release/ || true
            ;;
        "-s")
            rm -rf ../release/ || true
            ;;
        "-a")
            ALL_HOSTS=1
            ;;
        "-r")
            RUN_BUILD=1
            ;;

    esac

done

HOSTS=debian-11
if [ $ALL_HOSTS = 1 ]; then
    HOSTS="debian-11 debian-testing raspbian-32 raspbian-64"
fi

#determine if rebuild is required
if [ $RUN_TEST = 1 ]; then
    for HOST_STR in $HOSTS; do
        if [ !  -d ../release/$HOST_STR ] ; then
            RUN_BUILD=1
        fi
    done
fi

#And Run everything
if [ $RUN_BUILD = 1 ]; then
    gpg_unlock
fi

if [ $RUN_SOURCE = 1 ] || [ $RUN_BUILD = 1 ] || [ $RUN_TEST = 1 ]; then
    #automatic no-op if already done
    build_source
fi

for HOST_STR in $HOSTS; do
    (
        OS_TYPE=$(echo $HOST_STR | cut -d - -f 1)
        OS_VERSION=$(echo $HOST_STR | cut -d - -f 2)
        echo "Init $HOST_STR - $OS_TYPE - $OS_VERSION"
        . ./lib/settings.sh
        ret=0
        rm $OS_BASE_DIR/success || true
        #Only triggers rebuild (to force redownload use clean)
        if [ $RUN_IMAGE = 1 ]; then
            if [ $OS_TYPE = "debian" ]; then
                ./lib/boot-deb.sh debian $OS_VERSION rebuild 2>&1 | tee $OS_DIR/boot.log | sed "s/^/$HOST_STR: /"
                if [ "${PIPESTATUS[0]}" != "0" ]; then
                    ret=1
                fi

            elif [ $OS_TYPE = "raspbian" ]; then
                ./lib/boot-raspbian.sh raspbian $OS_VERSION rebuild 2>&1 | tee $OS_DIR/boot.log | sed "s/^/$HOST_STR: /"
                if [ "${PIPESTATUS[0]}" != "0" ]; then
                    ret=1
                fi

            fi
        fi

        #boots for all configs that need the working VM
        if [ $RUN_BOOT = 1 ] || [ $RUN_SOURCE = 1 ] || [ $RUN_BUILD = 1 ] || [ $RUN_TEST = 1 ]; then
            if [ $OS_TYPE = "debian" ]; then
                ./lib/boot-deb.sh $OS_VERSION boot 2>&1 | tee -a $OS_DIR/boot.log | sed "s/^/$HOST_STR: /"
                if [ "${PIPESTATUS[0]}" != "0" ]; then
                    ret=1
                fi

            elif [ $OS_TYPE = "raspbian" ]; then
                ./lib/boot-raspbian.sh $OS_VERSION boot 2>&1 | tee -a $OS_DIR/boot.log | sed "s/^/$HOST_STR: /"
                if [ "${PIPESTATUS[0]}" != "0" ]; then
                    ret=1
                fi

            fi
        fi

        if [ $RUN_BUILD = 1 ]; then
            if [ $OS_TYPE = "debian" ] || [ $OS_TYPE = "raspbian" ] ; then
                build_deb_pkg  | tee $OS_DIR/build.log 2>&1 | sed "s/^/$HOST_STR: /"
                if [ "${PIPESTATUS[0]}" != "0" ]; then
                    ret=1
                fi

            fi
        fi

        if [ $RUN_TEST = 1 ]; then
            test_install | tee $OS_DIR/test.log 2>&1 | sed "s/^/$HOST_STR: /"
            if [ "${PIPESTATUS[0]}" != "0" ]; then
                ret=1
            fi
        fi
        if [ "$ret" = "0" ] ; then
            touch $OS_BASE_DIR/success
        fi
    ) &
done
wait

echo 'All processes completed, results:'
echo
for HOST_STR in $HOSTS; do
    echo -en "\t$HOST_STR -- "
    if [[ -f $OS_BASE_DIR/$HOST_STR/success ]]; then
        echo 'Success!'
    else
        echo 'Failed'
    fi
done
