#!/bin/sh
set -e

# change to directory of the script
cd $(dirname "$0")

SKEL="$(pwd)/etc/skel"
echo "     source: $HOME"
echo "destination: $SKEL"

rm -rf "$SKEL"
mkdir -p "$SKEL/.config"
cd $HOME
cp -av .bashrc "$SKEL"
cp -av .xinitrc "$SKEL"
cp -rav .config/gtk-3.0 "$SKEL/.config"
cp -rav .config/xfce4 "$SKEL/.config"
cd "$SKEL"
ln -s .bashrc .profile
ln -s .xinitrc .xsession
