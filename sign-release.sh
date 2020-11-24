#!/bin/bash
set -e
ssh debbuild -t 'touch /tmp/chiton-sign && gpg -s --default-key chiton@edman007.com /tmp/chiton-sign && rm /tmp/chiton-sign*'
make clean
make -j15 release

cd release
gpg2 --armor --default-key chiton@edman007.com --clearsign *.dsc
mv *.dsc.asc *.dsc
for i in *.[xg]z ; do
	gpg2  --default-key  chiton@edman007.com  -b  $i
done

#and build
ssh debbuild 'rm -rf /home/edman007/chiton-pkg/*'
scp * debbuild:chiton-pkg
ssh debbuild 'export GPG_TTY=$(tty) && cd /home/edman007/chiton-pkg && dpkg-source -x chiton-*.dsc && cd chiton-*/ && debuild  -i -kchiton@edman007.com'
scp 'debbuild:chiton-pkg/*.deb' .

for i in *.deb ; do
	gpg2  --default-key  chiton@edman007.com  -b  $i
done
