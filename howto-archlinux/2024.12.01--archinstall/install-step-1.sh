#!/bin/sh
set -e

sudo pacman -S git
mkdir w
cd w
git clone https://github.com/calint/osca
