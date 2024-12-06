#!/bin/sh
set -e

sudo pacman --needed -S nano less libxft gtk4 thunar tumbler mousepad \
                        rhythmbox alsa-utils xorg-xsetroot scrot xorg-xinput \
                        ttf-dejavu htop feh

cd ~/w/osca
./make-all.sh
./init-user-home.sh