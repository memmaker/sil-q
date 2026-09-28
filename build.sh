#!/bin/sh
# Release build. sh build.sh unix  -> ./sil (curses, -mgcu)
#                sh build.sh win   -> ./sil.exe (MinGW cross, Windows frontend)
set -e
cd "$(dirname "$0")/src"
CORE="z-util.c z-virt.c z-form.c z-rand.c z-term.c variable.c tables.c util.c cave.c
	object1.c object2.c monster1.c monster2.c xtra1.c xtra2.c spells1.c spells2.c
	melee1.c melee2.c save.c files.c cmd1.c cmd2.c cmd3.c cmd4.c cmd5.c cmd6.c
	birth.c load.c squelch.c wizard1.c wizard2.c obj-info.c generate.c dungeon.c
	init1.c init2.c randart.c use-obj.c cmd-rvip.c"
if [ "$1" = win ]; then
	CROSS=${CROSS:-x86_64-w64-mingw32-}
	${CROSS}windres sil.rc -O coff -o sil.res
	${CROSS}gcc -O2 -w -fcommon -DWINDOWS -I. $CORE main-win.c readdib.c sil.res \
		-s -static -mwindows -lwinmm -lmsimg32 -o ../sil.exe
	rm -f sil.res
else
	${CC:-cc} -O2 -w -fcommon -DUSE_GCU -DUSE_NCURSES -I. $CORE main.c main-gcu.c -lncurses -o ../sil
fi
