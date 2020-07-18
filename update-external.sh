#!/bin/bash

#this goes through all external libs and updates them to the latest tag
cd external
for i in * ; do
    cd $i
    echo $i
    git checkout master
    git pull
    TARGET=`git tag | tail -n1`
    git checkout $TARGET
    cd ..
done
