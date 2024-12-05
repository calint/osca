#!/bin/sh
set -e

# update installation
sudo apt update
sudo apt upgrade

echo
echo install frameless
echo

sudo apt install git

mkdir w
cd w

git clone https://github.com/calint/osca

sudo apt install gcc libx11-dev libxft-dev gnome-devel dconf-cli scrot \
                 mousepad alsamixergui dconf-cli thunar feh alsa-utils \
                 xbacklight vlc rhythmbox
snap install brave

cd osca/
./make-all.sh
./init-user-home.sh
sudo cp -a usr/share/xsessions/frameless.desktop /usr/share/xsessions/

echo
echo logout and login to xsession frameless
echo
echo to configure touchpad see 1-after-login.sh
echo
echo or open terminal: key Super + c
echo xinput to find touchpad x
echo xinput list-props x to find properties:
echo - "Tapping Enabled" : y
echo - "Natural Scrolling Enabled" : z
echo xinput set-prop x y 1
echo xinput set-prop x z 1
echo save in script and run at login
echo
echo example for HP Stream Notebook PC 11
echo xinput set-prop 9 305 1
echo xinput set-prop 9 313 1
echo