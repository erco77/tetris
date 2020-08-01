tetris: tetris.c
	gcc -Wall tetris.c -o tetris

clean: FORCE
	(rm tetris; exit 0)

commit: FORCE
	git commit -a

FORCE:
