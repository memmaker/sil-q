#!/bin/sh
# Build Sil-Q for the browser (Emscripten + Asyncify).
# Output goes to web/dist; deploy with web/deploy.sh.
set -e
cd "$(dirname "$0")/.."
OUT=web/dist
rm -rf "$OUT" web/stage && mkdir -p "$OUT" web/stage/lib/xtra

# Game files: everything but the manuals, X11 fonts, sounds and BMP tiles
# (the browser draws from tiles.png, served next to the page)
for d in edit pref; do cp -R lib/$d web/stage/lib/; done
cp -R lib/xtra/tutorial web/stage/lib/xtra/
mkdir -p web/stage/lib/data web/stage/lib/save web/stage/lib/user web/stage/lib/apex
cp lib/xtra/graf/16x16_microchasm.png "$OUT/tiles.png"

SRCS=$(tr -d '\r' < src/Makefile.src | sed -n '/^ZFILES/,/^MAINFILES/p;/^ANGFILES/,/^$/p' \
	| grep -o '[a-z0-9_-]*\.o' | grep -v '^main' | grep -v '^maid' | sed 's/\.o$/.c/;s|^|src/|' | sort -u)

mkdir -p web/stage/lib/xtra/sound && cp lib/xtra/sound/sound.cfg web/stage/lib/xtra/sound/sound.cfg
emcc -O2 -fcommon -std=gnu99 -DUSE_WEB -Isrc -w \
	$SRCS src/main.c src/main-web.c \
	-o "$OUT/sil-core.js" \
	-sASYNCIFY -sASYNCIFY_STACK_SIZE=65536 -sSTACK_SIZE=1048576 \
	-sALLOW_MEMORY_GROWTH -sINITIAL_MEMORY=64MB \
	-sEXPORTED_FUNCTIONS=_main,_web_request_save \
	-sEXPORTED_RUNTIME_METHODS=FS,IDBFS,HEAPU8,addRunDependency,removeRunDependency \
	-sFORCE_FILESYSTEM -lidbfs.js -sENVIRONMENT=web \
	--preload-file web/stage/lib@/sil-q/lib

cp web/index.html "$HOME/Games/rvip-tools/web/rvip-wm.js" web/sil.js "$OUT/"
# Sound effects and music are fetched by the page, not preloaded
mkdir -p "$OUT/sound" && cp lib/xtra/sound/*.wav lib/xtra/sound/sound.cfg "$OUT/sound/"
mkdir -p "$OUT/music" && cp web/music/new_town.ogg "$OUT/music/"
# Game guide for the Help button, from ~/Desktop/Games/Roguelikes/Docs
python3 web/make-help.py > "$OUT/help.html"
rm -rf web/stage
ls -la "$OUT"
