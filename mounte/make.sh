#!/bin/sh
set -e

# change to directory of the script
cd $(dirname "$0")

CC="gcc"
#CC="clang -Weverything \
#    -Wno-declaration-after-statement \
#    -Wno-padded \
#    -Wno-reserved-identifier
#"
SRC=src/*.c
BIN=mounte
OPTS="-Os -Wfatal-errors -Werror -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion"
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
