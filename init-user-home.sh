#!/bin/sh
set -e
cd $(dirname "$0")

echo * copy '~/w/osca/etc/skel/.' to '~/'
cp -ra ~/w/osca/etc/skel/. ~/
echo * load dconf from '~/.config/dconf.ini'
dconf load / < ~/.config/dconf.ini
echo * delete '~/.config/dconf.ini'
rm ~/.config/dconf.ini
