mkdir -p part_management-1.0/DEBIAN
mkdir -p part_management-1.0/usr/local/bin
cp build/pmcli part_management-1.0/usr/local/bin/
cat <<EOF > part_management-1.0/DEBIAN/control
Package: pmcli
Version: 1.0
Section: base
Priority: optional
Architecture: amd64
Depends: libc6 (>= 2.3.4)
Maintainer: Markus Pfeifer <mkspfeifer@gmail.com.com>
Description: This program is used to organize my LEGO collection.
EOF
dpkg-deb --build part_management-1.0

