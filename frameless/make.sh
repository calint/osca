#CC=cc
CC="cc -std=c99"
BIN=frameless
SRC=src/frameless.c
# ! -Os breaks moving and resizing windows
OPTS="-Os -Werror -Wfatal-errors"
WARNINGS="-Wall -Wextra -Wpedantic -Wno-unused-result"
LIBS=-lX11

echo &&
$CC -o $BIN  $SRC $LIBS $OPTS $WARNINGS &&
echo    "            lines   words   chars" &&
echo -n "   source:" &&
cat $SRC|wc
echo -n "   zipped:" &&
cat $SRC|gzip|wc &&
echo && ls -o --color $BIN &&
echo