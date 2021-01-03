#!/bin/bash
INPUTF=license-meta.csv
NPM_DIR=external/npm
SRC_DIR=${1:-../..}/$NPM_DIR
if [ ! -r $INPUTF ]; then
    exit 1
fi

rm debian-license.in LICENSE.* || true
#make this a temp file so we don't have race conditions if it's run twice concurrently
TMP_TARGET=$(mktemp debian-license.in.XXXXXX )
while read line; do
    name=$(echo "$line" | cut -d , -f 2 | sed -e 's/^"//' -e 's/"$//')
    version=$(echo "$line" | cut -d , -f 3 | sed -e 's/^"//' -e 's/"$//' )
    licenses=$(echo "$line" | cut -d , -f 4 | sed -e 's/^"//' -e 's/"$//')
    copyright=$(echo "$line" | cut -d , -f 5 | sed -e 's/^"//' -e 's/"$//')
    licenseFile=$(echo "$line" | cut -d , -f 6 | sed -e 's/^"//' -e 's/"$//')
    url=$(echo "$line" | cut -d , -f 7 | sed -e 's/^"//' -e 's/"$//')
    email=$(echo "$line" | cut -d , -f 8 | sed -e 's/^"//' -e 's/"$//')
    publisher=$(echo "$line" | cut -d , -f 9 | sed -e 's/^"//' -e 's/"$//')

    if [ "$name" == "name" ]; then
        continue
    fi
    #copy the license file for later install
    if [ -r $licenseFile ]; then
        cp $licenseFile LICENSE.$name
    else
        cp $SRC_DIR/$licenseFile LICENSE.$name
    fi

    cp $licenseFile LICENSE.$name

    #make the debian license file
    echo >> $TMP_TARGET
    echo "Files: $NPM_DIR/node_modules/$name/*" >> $TMP_TARGET
    if [ "x$copyright" == "x" ]; then
        #make upp a copyright message
        if [ "x$publisher" != "x" ]; then
           copyright="$publisher $email"
        fi
    fi

    if [ "x$copyright" != "x" ]; then
        echo "Copyright: $copyright" >> $TMP_TARGET
    else
        #these are missing..Google needs to learn licensing.
        if [ "$name" == "pinch-zoom-element" ] || [ "$name" == "pointer-tracker" ]; then
            echo "Copyright: Google Chrome Labs 2020" >> $TMP_TARGET
        fi
    fi

    echo "Upstream-Name: $name" >> $TMP_TARGET
    echo "License: $name-$licenses" >> $TMP_TARGET
    sed -e 's/^/ /' -e 's/^\s*$/ ./' < LICENSE.$name >> $TMP_TARGET
done < $INPUTF
mv $TMP_TARGET debian-license.in
