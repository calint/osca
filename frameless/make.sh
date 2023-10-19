#!/bin/sh
set -e

# change to directory of the script
cd $(dirname "$0")

CC="cc"
#CC="clang -Weverything \
#    -Wno-sign-conversion \
#    -Wno-padded \
#    -Wno-declaration-after-statement \
#    -Wno-reserved-identifier"
BIN=frameless
SRC=src/frameless.c
OPTS="-Os -pedantic-errors -Wfatal-errors"
WARNINGS="-Wall -Wextra -Wno-unused-result"
        # -Wconversion 
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