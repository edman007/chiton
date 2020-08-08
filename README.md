
# Chiton

Chiton is a lightweight network video recorder. Chiton attempts to be fast and 
lightweight. It provides a web based interface to view live video of
all camera feeds and performs video recording and will avoid transcoding
whenever possible to maximize performance

## Installation

It is recommended to use a binary package for your distribition. After installation run `chiton-install` to configure chiton and then start it with `systemctl enable chiton` (on a systemd based system) or executing `/etc/rc.d/rc.chiton start` on a SysV based system (like Slackware).

If you would like to build from source, it is recommended to run the packaging script in the packaging directory to generate a binary for your distribution. If you downloaded this via git you will need to run `./autogen.sh` first.

Alternativitly you can install directly from source:

```bash
./autogen.sh #only if this was cloned from git
./configure --with-apacheconfigdir=/etc/apache2/sites-available/
make
make install
```

## Usage

Start the backend with `systemctl enable chiton` (on a systemd based system) or executing `/etc/rc.d/rc.chiton start` on a SysV based system (like Slackware).

Once started, you can access it via the web interface (http://localhost/chiton by default), and you can add your cameras via the web interface

## Contributing
Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

Please make sure to update tests as appropriate.

## License
[GPLv3](https://choosealicense.com/licenses/gpl-3.0/)

This program also contains code from other libraries, all are compatibe with GPLv3. See 3RD_PARTY and LICENSE.* for these licenses

