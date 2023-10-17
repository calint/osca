#!/bin/sh
set -e

# change to directory of the script
cd $(dirname "$0")

CC="gcc -std=c11"
SRC=src/*.c
BIN=mounte
OPTS="-Wfatal-errors -Wall -Wextra -Os"
$CC -o $BIN $OPTS $SRC
sudo chown root $BIN
sudo chmod u+s $BIN

echo
echo    "             lines  words   chars"
echo -n "   source:"
cat $SRC | wc
echo -n "   zipped:"
cat $SRC | gzip | wc
echo
ls -o $BIN
echo
