#!/bin/sh
set -e

# change to directory of the script
cd $(dirname "$0")

CC="gcc"
BIN=frameless
SRC=src/frameless.c
CF="-Wfatal-errors -Werror"
CW="-Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-unused-result -Wno-maybe-uninitialized"
CL="-lX11"
CMD="$CC $SRC -o $BIN $CL $CF $CW"
#echo $CMD
$CMD
echo
echo    "             lines  words   chars"
echo -n "   source:"
cat $SRC | wc
echo -n "   zipped:"
cat $SRC | gzip | wc
echo && ls -o --color $BIN
echo
