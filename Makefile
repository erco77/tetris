SHELL=/bin/sh

tetris: tetris.c
	gcc -Wall tetris.c -o tetris

clean: FORCE
	if [ -e tetris     ]; then rm tetris;     fi
	if [ -e tetris.obj ]; then rm tetris.obj; fi
	if [ -e tetris.exe ]; then rm tetris.exe; fi

commit: FORCE
	git commit -a

FORCE:
