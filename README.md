
# Chiton

Chiton is a lightweight network video recorder. Chiton attempts to be fast and 
lightweight. It provides a web based interface to view live video of
all camera feeds and performs video recording and will avoid transcoding
whenever possible to maximize performance

## Installation

It is recommended to use a binary package for your distribition. Then start it with `systemctl enable chiton` (on a systemd based system) or executing `/etc/rc.d/rc.chiton start` on a SysV based system (like Slackware).

A binary repository is available at https://repo.edman007.com/ and the GPG key is at https://repo.edman007.com/repo-key.pgp

On Debian based OS the gpg key can be added with

For debian/raspbian put the correct line into /etc/apt/sources.list.d/chiton.list
```
  #debian bullseye amd64
  deb https://repo.edman007.com/deb/debian/bullseye/ bullseye main

  #debian bookworm amd64
  deb https://repo.edman007.com/deb/debian/bookworm/ bookworm main

  #raspbian bullseye 32-bit
  deb https://repo.edman007.com/deb/rpi/bookworm/ bullseye main

  #raspbian 64-bit
  deb https://repo.edman007.com/deb/rpi/bullseye/ bullseye main
```
Then run the following commands to install chiton;

```
#Add the GPG key
wget -qO - https://repo.edman007.com/repo-key.pgp | sudo apt-key add -
#install mariadb-server and chiton
sudo apt install mariadb-server chiton
```

For Slackware, point slackpkg to the correct mirror:
```
  #Slackware 15.0
  https://repo.edman007.com/slack/slackware-15.0/

  #Slackware -current
  https://repo.edman007.com/slack/slackware-current/
```

Then install chiton (`slackpkg install chiton`), and configure it with `chiton-install`

If you would like to build from source, it is recommended to run the packaging script in the packaging directory to generate a binary for your distribution. If you downloaded this via git you will need to run `./autogen.sh` first. Note, when building from git node-license-checker is a build time dependency

For debian that would look something like this:

```bash
sudo apt-get install build-essential fakeroot devscripts
dpkg-source -x chiton-0.5.0~pre.dsc
cd chiton-0.5.0pre
sudo apt-get build-dep .
debuild -uc -us -i -b
cd ..
sudo dpkg -i chiton_0.5.0~pre-1_amd64.deb
```

For Slackware that would look something like this:

```bash
tar -xvf chiton-0.5.0pre.slackbuild.tar.xz
mv chiton-0.5.0pre.tar.xz chiton
./chiton.SlackBuild
sudo upgradepkg --install-new /tmp/chiton-0.5.0pre*.txz
chiton-install
chmod +x /etc/rc.d/rc.chiton
/etc/rc.d/rc.chiton start
```

Alternativitly you can install directly from source:

```bash
./autogen.sh #only if this was cloned from git
./configure --help #read the options, you do want to set your system directories
make
make install
chiton-install
systemctl start chiton
```

## Usage

Start the backend with `systemctl start chiton` (on a systemd based system) or executing `/etc/rc.d/rc.chiton start` on a SysV based system (like Slackware).

Once started, you can access it via the web interface (http://localhost/chiton by default), and you can add your cameras via the web interface

To configure, go to settings, add a camera, "active" must be set to "1" and it must have a "video-url" parameter set,
see chiton_config.hpp for documentation on the options, you can then reload the daemon to activte the new settings (`systemctl reload chiton`)

## Contributing
Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

## License
[GPLv3](https://choosealicense.com/licenses/gpl-3.0/)

This program also contains code from other libraries, all are compatibe with GPLv3. See 3RD_PARTY and LICENSE.* for these licenses

