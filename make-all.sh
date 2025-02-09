#!/bin/sh
# tools used:
#   gcc      14.2.1
set -e

# change to directory of the script
cd $(dirname "$0")

for f in frameless clonky menuq stickyo snap; do
    cd $f
    ./make.sh
    cd ..
done
