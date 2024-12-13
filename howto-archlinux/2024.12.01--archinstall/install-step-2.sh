#!/bin/sh
set -e

sudo pacman --needed -S xterm xorg-xsetroot xorg-xinput libxft thunar tumbler \
                        mousepad gtk4 rhythmbox alsa-utils scrot feh acpi \
                        sysstat brightnessctl ttf-dejavu bluez bluez-utils

cd ~/w/osca
./init-user-home.sh
./make-all.sh
