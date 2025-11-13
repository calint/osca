#!/bin/sh
set -e
cd "$(dirname "$0")"

SKEL="$(pwd)/etc/skel"

echo "     source: $HOME"
echo "destination: $SKEL"

cd "$HOME"
cp -av .bashrc "$SKEL"
cp -av .xinitrc "$SKEL"
cp -av .Xresources "$SKEL"
cd "$SKEL"
ln -sf .bashrc .profile
ln -sf .xinitrc .xsession
