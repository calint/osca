#!/bin/sh
set -e

# change to directory of the script
cd "$(dirname "$0")"

{
    cat README.1.md
    echo
    echo '```'
    ./make-all.sh
    echo '```'
} >README.md
