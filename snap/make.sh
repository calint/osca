#!/bin/bash
set -e

# change to directory of the script
cd "$(dirname "$0")"

BIN=snap
SRC=src/snap.c
CC="gcc"
#CC="clang -Weverything \
#    -Wno-disabled-macro-expansion \
#    -Wno-padded \
#    -Wno-declaration-after-statement \
#    -Wno-format-nonliteral \
#    -Wno-missing-noreturn \
#    -Wno-reserved-identifier \
#    -Wno-documentation \
#    -Wno-documentation-unknown-command \
#"
CF="-Os -Wfatal-errors -Werror"
CW="-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion"
CMD="$CC $SRC -o $BIN $CF $CW -I/usr/include/cairo/ -lX11 -lcairo"
#echo
#echo $CMD
$CMD
echo
echo "            lines   words   chars"
echo -n "   source:"
cat $SRC | wc
echo -n "   zipped:"
cat $SRC | gzip | wc
echo
ls -o $BIN
echo
