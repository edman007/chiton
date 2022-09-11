#!/bin/bash
set -e

#we are passed the KEY & PASS
read -s GPG_SIGN_KEY
read -s GPG_SIGN_PASS
cd ~/pkg
sudo apt-get update
sudo apt-get upgrade -y
sudo apt-get install -y build-essential fakeroot devscripts
dpkg-source -x chiton-*.dsc
cd chiton-*/
if [ "x$GPG_SIGN_KEY" = "x" ]; then
    SIGN_PARAMS=
else
    touch ../gpg-pass
    trap 'rm ../gpg-pass' EXIT
    chmod 600 ../gpg-pass
    cat - <<< $GPG_SIGN_PASS > ../gpg-pass
    cat - > ../gpg-cmd <<EOF
#!/bin/bash
#echo "gpg --batch --pinentry-mode loopback --passphrase-file /home/chiton-build/pkg/gpg-pass \$@"
#cat /home/chiton-build/pkg/gpg-pass
gpg --batch --pinentry-mode loopback --passphrase-file /home/chiton-build/pkg/gpg-pass \$@
EOF
    chmod +x ../gpg-cmd
    SIGN_PARAMS="-k$GPG_SIGN_KEY -p/home/chiton-build/pkg/gpg-cmd"
fi
sudo apt-get build-dep -y .
debuild  -i $SIGN_PARAMS
