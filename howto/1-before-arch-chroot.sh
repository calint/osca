#!/bin/sh
set -e
cd $(dirname "$0")

cp /etc/systemd/network/* /mnt/etc/systemd/network/ 
