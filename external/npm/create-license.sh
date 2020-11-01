#!/bin/bash
INPUTF=license-meta.csv
NPM_DIR=external/npm
if [ ! -r $INPUTF ]; then
    exit 1
fi
if [ ! -r license-meta.csv ]; then
    exit 2
fi

rm debian-license.in LICENSE.* || true
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
    cp $licenseFile LICENSE.$name

    #make the debian license file
    echo "Files: $NPM_DIR/node_modules/$name/*" >> debian-license.in
    if [ "x$copyright" == "x" ]; then
        #make upp a copyright message
        if [ "x$publisher" != "x" ]; then
           copyright="$publisher $email"
        fi
    fi

    if [ "x$copyright" == "x" ]; then
        echo "Copyright: $copyright" >> debian-license.in
    fi

    echo "Upstream-Name: $name" >> debian-license.in
    echo "License: $licenses" >> debian-license.in
    sed -e 's/^/ /' -e 's/^\s*$/ ./' < $licenseFile >> debian-license.in
done < $INPUTF
