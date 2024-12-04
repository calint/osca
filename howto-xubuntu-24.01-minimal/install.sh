#!/bin/sh
set -e

# update installation
sudo apt update
sudo apt upgrade

# install frameless
mkdir w
cd w
git clone https://github.com/calint/osca
sudo apt install gcc libx11-dev libxft-dev gnome-devel dconf-cli scrot \
                 mousepad alsamixergui dconf-cli thunar feh alsa-utils \
                 xbacklight vlc rhythmbox
snap install brave
cd osca/
./make-all.sh
sudo cp -a usr/share/xsessions/frameless.desktop /usr/share/xsessions/
./init-user-home.sh

# logout and login to xsession frameless