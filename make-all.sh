#!/bin/sh
set -e

# change to directory of the script
cd $(dirname "$0")

for f in frameless clonky menuq stickyo snap mounte; do
    cd $f
    ./make.sh
    cd ..
done
