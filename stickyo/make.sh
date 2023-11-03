#!/bin/sh
set -e

# change to directory of the script
cd $(dirname "$0")

BIN=stickyo
SRC=src/stickyo.c
CC="gcc"
#CC="clang -Weverything"
CF="-Os -Wfatal-errors"
CW="-Wall -Wextra -Wno-unused-parameter -Wconversion -Wsign-conversion"
CMD="$CC $SRC -o $BIN $CF $CW $(pkg-config --cflags --libs gtk4)"
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
