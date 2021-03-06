
# Chiton

Chiton is a lightweight network video recorder. Chiton attempts to be fast and 
lightweight. It provides a web based interface to view live video of
all camera feeds and performs video recording and will avoid transcoding
whenever possible to maximize performance

## Installation

It is recommended to use a binary package for your distribition. Then start it with `systemctl enable chiton` (on a systemd based system) or executing `/etc/rc.d/rc.chiton start` on a SysV based system (like Slackware).

If you would like to build from source, it is recommended to run the packaging script in the packaging directory to generate a binary for your distribution. If you downloaded this via git you will need to run `./autogen.sh` first. Note, when building from git node-license-checker is a build time dependency

For debian that would look something like this:

```bash
dpkg-source -x chiton-0.1.0git.dsc
cd chiton-0.1.0git
debuild -uc -us -i -b
cd ..
sudo dpkg -i chiton_0.1.0git-1_amd64.deb
```

For Slackware that would look something like this:

```bash
tar -xvf chiton-0.1.0git.slackbuild.tar.xz
mv chiton-0.1.0git.tar.xz chiton
./chiton.SlackBuild
sudo upgradepkg --install-new /tmp/chiton-0.1.0git*.txz
```

Alternativitly you can install directly from source:

```bash
./autogen.sh #only if this was cloned from git
./configure --help #read the options, you do want to set your system directories
make
make install
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

