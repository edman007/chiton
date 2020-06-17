#!/bin/bash

TOP_DIR=`pwd`

#Smarty
rm -rf web/inc/external/smarty
cd web/inc/external/smarty.git
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

cd web/inc/external/hls.js.git
rm -rf dist
git checout master
git pull
TARGET_HLS=`git tag | tail -n1`
echo "Installing HLS $TARGET_HLS"
git checkout $TARGET_HLS
npm install
npm run build
cp dist/hls* $TOP_DIR/web/static/
cp LICENSE $TOP_DIR/LICENSE.hls
