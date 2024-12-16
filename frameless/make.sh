#!/bin/sh
set -e

# change to directory of the script
cd $(dirname "$0")

CC="gcc"
#CC="clang -Weverything -Wno-declaration-after-statement -Wno-padded -Wno-unsafe-buffer-usage"
BIN=frameless
SRC=src/frameless.c
OPTS="-Os -Wfatal-errors -Werror"
WARNINGS="-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wno-unused-result -Wno-unused-parameter"
LIBS="-lX11"

echo
$CC -o $BIN  $SRC $LIBS $OPTS $WARNINGS
echo    "            lines   words   chars"
echo -n "   source:"
cat $SRC | wc
echo -n "   zipped:"
cat $SRC | gzip | wc
echo
ls -o $BIN
echo