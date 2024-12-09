#!/bin/sh
set -e

sudo pacman --needed -S nano less xterm libxft gtk4 thunar tumbler mousepad \
                        rhythmbox alsa-utils xorg-xsetroot scrot xorg-xinput \
                        ttf-dejavu htop feh acpi sysstat bluez bluez-utils \
                        brightnessctl man

cd ~/w/osca
./init-user-home.sh
./make-all.sh
