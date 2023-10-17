#!/bin/sh
set -e

# change to directory of the script
cd $(dirname "$0")

BIN=snap
SRC=src/snap.c

CF="-Os -Wfatal-errors -Werror"
CW="-Wall -Wextra -Wpedantic"
CMD="gcc $SRC -o $BIN $CF $CW -I/usr/include/cairo/ -lX11 -lcairo"
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
