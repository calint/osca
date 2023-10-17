#!/bin/sh
set -e

# change to directory of the script
cd $(dirname "$0")

#CC=cc
CC="cc -std=c99"
BIN=frameless
SRC=src/frameless.c
OPTS="-Os -pedantic-errors -Wfatal-errors"
WARNINGS="-Wall -Wextra -Wno-unused-result"
LIBS="-lX11"

echo
$CC $SRC -o $BIN $LIBS $OPTS $WARNINGS
echo    "            lines   words   chars"
echo -n "   source:"
cat $SRC | wc
echo -n "   zipped:"
cat $SRC | gzip | wc
echo
ls -o --color $BIN
echo
