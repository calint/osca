#!/bin/sh
set -e
cd $(dirname "$0")

pacman -S git
mkdir -p /home/c/w
cd /home/c/w
git clone https://github.com/calint/osca
