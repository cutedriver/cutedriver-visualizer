Debian package control file is located in:
debian/
Change it's values accordingly!

Files to be installed must be located in:
debian/install-dir

Build deb package using command:
dpkg-buildpackage -rfakeroot -b
