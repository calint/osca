#!/bin/sh
set -e
cd "$(dirname "$0")"

echo · copy '~/w/osca/etc/skel/.' to '~/'
cp -ra ~/w/osca/etc/skel/. ~/
echo · load dconf from '~/dconf.ini'
dconf load / <~/dconf.ini
echo · delete '~/dconf.ini'
rm ~/dconf.ini
