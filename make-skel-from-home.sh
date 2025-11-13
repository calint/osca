#!/bin/sh
set -e
cd "$(dirname "$0")"

SKEL="$(pwd)/etc/skel"

echo "     source: $HOME"
echo "destination: $SKEL"

mkdir -p "$SKEL/.config"
cd "$HOME"
cp -av .bashrc "$SKEL"
cp -av .xinitrc "$SKEL"
cp -av .Xresources "$SKEL"
cd "$SKEL"
rm -f .profile
rm -f .xsession
ln -s .bashrc .profile
ln -s .xinitrc .xsession
