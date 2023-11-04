#!/bin/sh
set -e
cd $(dirname "$0")

# update packages database
pacman -Sy

# install git
pacman -S git

# create workspace
mkdir -p /home/c/w

cd /home/c/w

# clone osca
git clone https://github.com/calint/osca
