#!/bin/sh
set -e
cd $(dirname "$0")

sudo pacman -S --noconfirm dconf

../init-user-home.sh

source ~/.bashrc

# X11
sudo pacman -S --noconfirm xorg xorg-xinit

# clonky dependencies
sudo pacman -S --noconfirm libxft acpi sysstat ttf-dejavu

# stickyo dependencies
sudo pacman -S --noconfirm pkg-config gtk4

# compiler
sudo pacman -S --noconfirm gcc

# compile all
~/w/osca/make-all.sh

# frameless launched applications
sudo pacman -S --noconfirm xterm thunar mousepad rhythmbox alsa-utils firefox scrot

# miscellaneous utils
sudo pacman -S --noconfirm wget htop feh pkgfile

echo
echo start X11 with 'start-frameless'
echo
