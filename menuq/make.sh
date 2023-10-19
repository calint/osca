#!/bin/sh
set -e

# change to directory of the script
cd $(dirname "$0")

BIN=menuq
SRC=src/menuq.c
CC="gcc"
#CC="clang -Weverything -Wno-declaration-after-statement"
CF="-Os -Wfatal-errors -Werror"
CW="-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion"
LIBS="-lX11"
CMD="$CC $SRC -o $BIN $CF $CW $LIBS"
$CMD
echo
echo    "            lines   words   chars"
echo -n "   source:"
cat $SRC | wc
echo -n "   zipped:"
cat $SRC | gzip | wc
echo && ls -o $BIN
echo
#valgrind --leak-check=yes --leak-check=full --show-leak-kinds=all ./$BIN
#valgrind --leak-check=yes --leak-check=full ./$BIN
#valgrind --leak-check=yes ./$BIN
