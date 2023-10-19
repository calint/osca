#!/bin/sh
set -e

# change to directory of the script
cd $(dirname "$0")

BIN=stickyo
SRC=src/stickyo.c
CC="gcc"
#CC="clang -Weverything"
CF="-Os -Wfatal-errors -Werror"
CW="-Wall -Wextra -Wpedantic -Wno-unused-parameter -Wconversion -Wsign-conversion"
CMD="gcc $SRC -o $BIN $CF $CW $(pkg-config --cflags --libs gtk+-3.0)"
#CMD="gcc $SRC -o $BIN $CF $CW $(pkg-config --cflags --libs gtk4)"
#echo
#echo $CMD
$CMD
echo
echo    "            lines   words   chars"
echo -n "   source:"
cat $SRC | wc
echo -n "   zipped:"
cat $SRC | gzip | wc
echo
ls -o $BIN
echo
