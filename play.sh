#!/bin/sh
# Sil-Q, X11 frontend: MicroChasm 16x16 tiles (-g) drawn in big-tile mode
# (-b, 15x32 font -> 30x32 map cells), scaled nearest-neighbour (-s), and
# extra windows: 1 inventory, 3 monster list, 5 combat rolls, 2 messages,
# 4 monster recall (contents set in lib/pref/user-x11.prf).
# Screen 1440x932; XQuartz adds a ~28px title bar above each window.
export SIL_X11_FONT_0='-misc-fixed-medium-r-normal--32-*-*-*-c-150-iso8859-1'
export SIL_X11_AT_X_0=0    SIL_X11_AT_Y_0=0                                                   # main 80x24
export SIL_X11_FONT_1=6x10 SIL_X11_AT_X_1=1206 SIL_X11_AT_Y_1=0   SIL_X11_COLS_1=39  SIL_X11_ROWS_1=32  # inventory
export SIL_X11_FONT_3=6x10 SIL_X11_AT_X_3=1206 SIL_X11_AT_Y_3=356 SIL_X11_COLS_3=39  SIL_X11_ROWS_3=19  # monster list
export SIL_X11_FONT_5=6x10 SIL_X11_AT_X_5=1206 SIL_X11_AT_Y_5=580 SIL_X11_COLS_5=39  SIL_X11_ROWS_5=19  # combat rolls
export SIL_X11_FONT_2=8x13 SIL_X11_AT_X_2=0    SIL_X11_AT_Y_2=800 SIL_X11_COLS_2=107 SIL_X11_ROWS_2=7   # messages
export SIL_X11_FONT_4=8x13 SIL_X11_AT_X_4=862  SIL_X11_AT_Y_4=800 SIL_X11_COLS_4=72  SIL_X11_ROWS_4=7   # monster recall
cd "$(dirname "$0")" && exec ./sil -mx11 "$@" -- -n6 -g -b -s
