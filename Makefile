.c.o:
	g++ -c -Wall $<

all : str_lib.o cli.o
	mkdir -p build
	g++ -o build/pmcli -std=c++17 cli.o str_lib.o -lpqxx -lpq

test : str_lib.o test.o
	mkdir -p build
	g++ -o build/test -std=c++17 str_lib.o test.o

deb : all
	mkdir -p part_management-1.0/DEBIAN
	mkdir -p part_management-1.0/usr/local/bin
	cp build/pmcli part_management-1.0/usr/local/bin/
	echo "Package: pmcli" > part_management-1.0/DEBIAN/control
	echo "Version: 1.0" >> part_management-1.0/DEBIAN/control
	echo "Section: base" >> part_management-1.0/DEBIAN/control
	echo "Priority: optional" >> part_management-1.0/DEBIAN/control
	echo "Architecture: amd64" >> part_management-1.0/DEBIAN/control
	echo "Depends: libc6 (>= 2.3.4)" >> part_management-1.0/DEBIAN/control
	echo "Maintainer: Markus Pfeifer <mkspfeifer@gmail.com>" >> part_management-1.0/DEBIAN/control
	echo "Description: This program is used to organize my LEGO collection." >> part_management-1.0/DEBIAN/control
	dpkg-deb --build part_management-1.0

str_lib.o : str_lib.cpp
cli.o : cli.cpp
test.o : test.cpp

clean :
	rm -f *.o
	rm -fr build
	rm -fr part_management-1.0
	rm -f part_management-1.0.deb

