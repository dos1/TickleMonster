### Tickle Monster vs Suits

Dependences (for Debian-based distros):

	sudo apt install liballegro5.0 liballegro-ttf5.0 liballegro-image5.0 liballegro-audio5.0 liballegro-acodec5.0

The game uses CMake as build system, so its building process is pretty typical.

Building dependences:

	sudo apt install liballegro5-dev liballegro-ttf5-dev liballegro-image5-dev liballegro-audio5-dev liballegro-acodec5-dev

Compilation:

	mkdir build; cd build
	cmake ..
	make

Running (from top directory):

	build/src/ticklemonster

Installation (as root):

	make install

For packaging information, read lib/README.txt

https://github.com/dos1/TickleMonster/

Written by Sebastian Krzyszkowiak <dos@dosowisko.net>
