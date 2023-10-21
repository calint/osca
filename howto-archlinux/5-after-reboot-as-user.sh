#!/bin/sh
set -e
cd $(dirname "$0")

../init-user-home.sh

source ~/.bashrc

# compiler
sudo pacman -S --noconfirm gcc

# X11
sudo pacman -S --noconfirm xorg xorg-xinit xterm

# clonky dependencies
sudo pacman -S --noconfirm libxft acpi sysstat

# stickyo dependencies
sudo pacman -S --noconfirm pkg-config gtk3

# compile all
~/w/osca/make-all.sh

# frameless launched applications
sudo pacman -S --noconfirm firefox thunar mousepad alsa-utils

# utils
sudo pacman -S --noconfirm wget htop feh scrot pkgfile ttf-dejavu
# faenza-icon-theme 

echo
echo start X11 with 'start-frameless'
echo