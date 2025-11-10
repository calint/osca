#!/bin/sh
set -e

sudo pacman --needed -S xterm xorg-xsetroot xorg-xinput libxft thunar tumbler \
    mousepad gtk4 rhythmbox mpv alsa-utils scrot feh eog acpi sysstat iw \
    brightnessctl ttf-dejavu bluez bluez-utils gvfs gvfs-mtp iw tlp powertop

cd ~/w/osca
./init-user-home.sh
./make-all.sh

sudo systemctl enable bluetooth
sudo systemctl start bluetooth
sudo systemctl enable tlp
sudo systemctl start tlp
