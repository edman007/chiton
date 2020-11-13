#!/bin/bash
cd release
gpg --armor --default-key chiton@edman007.com --clearsign *.dsc
mv *.dsc.asc *.dsc
for i in *.[xg]z ; do
	gpg-sign  --default-key  chiton@edman007.com  -b  $i
done
