#!/bin/sh
set -e
cd $(dirname "$0")

echo assuming ethernet is \'enp0s3\'
echo modify the script with actual ethernet interface if necessary
echo run \'ip link\' and find out

ip link set enp0s3 up
systemctl enable systemd-networkd
systemctl start systemd-networkd
systemctl status systemd-networkd
systemctl enable systemd-resolved
systemctl start systemd-resolved
systemctl status systemd-resolved

