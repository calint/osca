#!/bin/sh
set -e
cd $(dirname "$0")

cp -rav ~/w/osca/etc/skel/. ~/
echo load dconf from '~/w/osca/etc/skel/dconf.ini'
dconf load / < ~/w/osca/etc/skel/dconf.ini

