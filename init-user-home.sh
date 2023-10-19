#!/bin/sh
set -e
cd $(dirname "$0")

cp -rav ~/w/osca/etc/skel/. ~/
dconf load / < ~/w/osca/etc/skel/dconf.ini

