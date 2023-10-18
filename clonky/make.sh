#!/bin/sh
set -e

# change to directory of the script
cd $(dirname "$0")

CC="gcc"
#CC=cc
BIN=clonky
SRC=src/*.c
#OPTS="-Os -pedantic-errors -Wfatal-errors"
OPTS="-Os -z execstack"
CW="-Wall -Wextra -Wno-unused-result -Wno-unused-function"
#  -Wpedantic
LIBS="-lX11 -lXft"
INCLUDES=-I/usr/include/freetype2/
CMD="$CC $SRC -o $BIN $OPTS $LIBS $CW $INCLUDES"
echo $CMD
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
#valgrind --track-origins=yes --leak-check=full ./$BIN