#!/bin/sh
set -e
cd $(dirname "$0")

cp -rav ~/w/osca/etc/skel/. ~/

source ~/.bashrc

# compiler
sudo pacman -S --noconfirm gcc

# X11
sudo pacman -S --noconfirm xorg xorg-xinit xterm

# frameless launched applications
sudo pacman -S --noconfirm chromium thunar leafpad alsa-utils

# stickyo
sudo pacman -S --noconfirm pkg-config gtk3

# utils
sudo pacman -S --noconfirm wget pcmanfm feh scrot mplayer pkgfile ttf-dejavu
# faenza-icon-theme 

# clonky dependencies
sudo pacman -S --noconfirm libxft acpi sysstat

./make-all.sh

echo
echo start X11 with 'start-frameless'
echo