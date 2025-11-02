#!/bin/sh
set -e

sudo pacman -Syu
sudo pacman -S git
mkdir w
cd w
git clone https://github.com/calint/osca
