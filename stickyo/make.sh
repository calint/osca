#!/bin/sh
set -e

# change to directory of the script
cd $(dirname "$0")

BIN=stickyo
SRC=src/stickyo.c
CF="-Os -Wfatal-errors -Werror"
CW="-Wall -Wextra -Wpedantic -Wno-unused-parameter"
CMD="gcc $SRC -o $BIN $CF $CW `pkg-config --cflags --libs gtk+-3.0`"
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
