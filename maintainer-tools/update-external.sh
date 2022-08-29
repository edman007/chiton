#!/bin/bash
if [ ! -d maintainer-tools ] ; then
  cd ..
fi
#this goes through all external libs and updates them to the latest tag
cd external
for i in * ; do
    if [ "$i" == "npm" ]; then
        continue;
    fi
    cd $i
    echo $i
    git checkout master
    git pull
    TARGET=`git tag | tail -n1`
    git checkout $TARGET
    cd ..
done
cd external/npm
npm update hls.js
