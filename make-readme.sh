#!/bin/sh
set -e

# change to directory of the script
cd $(dirname "$0")

cat README.1.md > README.md
echo >> README.md
echo '```' >> README.md
./make-all.sh >> README.md
echo '```' >> README.md
