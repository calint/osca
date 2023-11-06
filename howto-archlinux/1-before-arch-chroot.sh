#!/bin/sh
set -e
cd $(dirname "$0")

# copy network settings
cp /etc/systemd/network/* /mnt/etc/systemd/network/ 
