#!/bin/sh
set -e
cd $(dirname "$0")

mkdir w
cd w
git clone https://github.com/calint/osca
cd osca
cp -rav etc/skel/. ~/

source ~/.bashrc

# compiler
sudo pacman -S gcc

# X11
sudo pacman -S xorg xorg-xinit xterm

# clonky
sudo pacman -S libxft acpi sysstat

# frameless launched applications
sudo pacman -S chromium thunar leafpad ttf-dejavu

# sticky
sudo pacman -S pkg-config gtk3

cd /home/c/w/osca
sh make-all.sh

sudo pacman -S wget pcmanfm feh scrot mplayer pkgfile alsa-utils
# faenza-icon-theme 
