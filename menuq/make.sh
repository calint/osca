#!/bin/sh
set -e

# change to directory of the script
cd $(dirname "$0")

BIN=menuq
SRC=src/menuq.c
CC="gcc"
#CC="clang -Weverything -Wno-declaration-after-statement -Wno-unsafe-buffer-usage"
OPTS=
#OPTS="-g"
CF="-Os -Wfatal-errors -Werror"
CW="-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion"
LIBS="-lX11 -lXft"
INCLUDES="-I/usr/include/freetype2/"
CMD="$CC $SRC -o $BIN $OPTS $CF $CW $LIBS $INCLUDES"
$CMD
echo
echo    "            lines   words   chars"
echo -n "   source:"
cat $SRC | wc
echo -n "   zipped:"
cat $SRC | gzip | wc
echo && ls -o $BIN
echo
#valgrind --leak-check=full --show-leak-kinds=all ./$BIN
