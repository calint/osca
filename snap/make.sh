#!/bin/sh
set -e

# change to directory of the script
cd $(dirname "$0")

BIN=snap
SRC=src/snap.c

CMD="gcc $SRC -o $BIN -Os -I/usr/include/cairo/ -lX11 -lcairo"
$CMD
echo
echo    "             lines  words   chars"
echo -n "   source:"
cat $SRC | wc
echo -n "   zipped:"
cat $SRC | gzip | wc
echo
ls -o --color $BIN
echo
