#!/bin/sh
set -e

# change to directory of the script
cd $(dirname "$0")

BIN=stickyo
SRC=src/stickyo.c

gcc -o $BIN $SRC -Wfatal-errors -Wall -Wextra -Wpedantic -Wno-deprecated-declarations `pkg-config --cflags --libs gtk+-3.0`

echo
echo    "             lines  words   chars"
echo -n "   source:"
cat $SRC | wc
echo -n "   zipped:"
cat $SRC | gzip | wc
echo
ls -o --color $BIN
echo
