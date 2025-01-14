#!/bin/sh
set -e

sudo pacman --needed -S xterm xorg-xsetroot xorg-xinput libxft thunar tumbler \
                        mousepad gtk4 rhythmbox mpv alsa-utils scrot feh eog \
                        acpi sysstat brightnessctl ttf-dejavu bluez bluez-utils \
                        gvfs gvfs-mtp

cd ~/w/osca
./init-user-home.sh
./make-all.sh
