.cpp.o:
	g++ -c -std=c++17 $<

all : str_lib.o main.o csv.o pugixml.o item.o
	mkdir -p build
	g++ -o build/pmcli -std=c++17 main.o str_lib.o csv.o pugixml.o item.o -lpqxx -lpq -lncurses

test : str_lib.o test.o
	mkdir -p build
	g++ -o build/test -std=c++17 str_lib.o test.o

DEBIAN_PATH=pmcli-1.0/DEBIAN
CONTROL_FILE=$(DEBIAN_PATH)/control
BIN_PATH=pmcli-1.0/usr/local/bin

deb : all
	mkdir -p $(DEBIAN_PATH)
	mkdir -p $(BIN_PATH)
	cp build/pmcli $(BIN_PATH)
	echo "Package: pmcli" > $(CONTROL_FILE)
	echo "Version: 1.0" >> $(CONTROL_FILE)
	echo "Section: base" >> $(CONTROL_FILE)
	echo "Priority: optional" >> $(CONTROL_FILE)
	echo "Architecture :amd64" >> $(CONTROL_FILE)
	echo "Depends: libc6 (>= 2.3.4)" >> $(CONTROL_FILE)
	echo "Maintainer: Markus Pfeifer <mkspfeifer@gmail.com>" >> $(CONTROL_FILE)
	echo "Description: This program is used to organize my LEGO collection." >> $(CONTROL_FILE)
	dpkg-deb --build pmcli-1.0

str_lib.o : str_lib.cpp
main.o : main.cpp
test.o : test.cpp
csv.o : csv.cpp
item.o : item.cpp
pugixml.o : libs/pugixml-1.15/src/pugixml.cpp
	g++ -c libs/pugixml-1.15/src/pugixml.cpp -std=c++17 -Ilibs/pugixml-1.15/src -o pugixml.o

clean :
	rm -f *.o
	rm -fr build
	rm -fr pmcli-1.0
	rm -f pmcli-1.0.deb

