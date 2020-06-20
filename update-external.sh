#!/bin/bash

if [ $# == 0 ]; then
    echo "Usage: ./update-external.sh [project]"
    echo "projects:"
    echo -e "\tsmarty"
    echo -e "\thls"
    echo -e "\tbreeze-icons"
    exit
fi

TOP_DIR=`pwd`
if [ "x$1" == "xsmarty" ]; then
    #Smarty
    rm -rf web/inc/external/smarty
    cd $TOP_DIR/external/smarty.git
    git checkout master
    git pull
    TARGET_SMARTY=`git tag | tail -n1`
    echo "Installing Smarty $TARGET_SMARTY"
    git checkout $TARGET_SMARTY
    cp -r libs $TOP_DIR/web/inc/external/smarty
    #this is the version tag the offical release used, we track it so we use this
    sed -i "s/const SMARTY_VERSION = '[^']\+';/const SMARTY_VERSION = '$1';/" $TOP_DIR/web/inc/external/smarty/Smarty.class.php
    cp LICENSE $TOP_DIR/LICENSE.smarty
    cd $TOP_DIR
fi

if [ "x$1" == "xhls" ]; then
    #this requires npm
    #HLS.js
    cd $TOP_DIR/external/hls.js.git
    rm -rf dist
    git checout master
    git pull
    TARGET_HLS=`git tag | tail -n1`
    echo "Installing HLS $TARGET_HLS"
    git checkout $TARGET_HLS
    npm install
    npm run build
    cp dist/hls.* $TOP_DIR/web/static/
    cp LICENSE $TOP_DIR/LICENSE.hls
    #clean to stop tracking the changes the build made
    rm -rf dist package-lock.json
    cd $TOP_DIR
fi

if [ "x$1" == "xbreeze-icons" ]; then
    #breeze-icons
    cd $TOP_DIR/external/breeze-icons.git
    git checkout master
    git pull
    TARGET_BREEZE=`git tag | tail -n1`
    echo "Installing breeze-icons $TARGET_BREEZE"
    git checkout $TARGET_BREEZE
    rm -rf $TOP_DIR/web/static/breeze
    mkdir -p $TOP_DIR/web/static/breeze
    cp ./icons/status/symbolic/media-*.svg $TOP_DIR/web/static/breeze
    cp ./icons/actions/symbolic/media-*.svg $TOP_DIR/web/static/breeze
    (cd $TOP_DIR/web/static/breeze/ ;
     for i in *.svg ; do
         target=`echo $i | sed -e s/media-// -e s/-symbolic// -e s/.svg/.png/`
         echo "rasterizing $i"
         convert -density 288 $i  4x-$target &
         convert -density 144 $i  2x-$target
     done
    )
    cp COPYING.LIB $TOP_DIR/LICENSE.breeze-icons
    cd $TOP_DIR
fi
