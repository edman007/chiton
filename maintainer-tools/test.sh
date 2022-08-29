#!/bin/bash
OS_TYPE=raspbian
. ./lib/settings.sh
show_help () {
    echo -e "$0 <arg>\n"
    echo -e "\thelp - show this help"
    echo -e "\tbuild - build golden packages (x84-64)"
    echo -e "\ttest - build and test golden packages (x84-64)"
    echo -e "\tbuild-all - build golden packages (x84-64)"
    echo -e "\ttest-all - build and test golden packages (x84-64)"

    exit 0
}



build_arm () {
    ./lib/boot-raspbian.sh 32 freshen
    ./lib/boot-raspbian.sh 64 freshen
}


build_debian () {
    ./lib/boot-deb.sh 11 freshen
    ./lib/boot-deb.sh testing freshen
}

test_install () {
    #$1 OS
    #$2 VERSION

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
