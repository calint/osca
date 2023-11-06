#!/bin/sh
set -e
cd $(dirname "$0")

# install some basic text editors and sudo
pacman -S --noconfirm vi nano sudo

# create user 'c' and home directory
useradd -d /home/c c
chown -R c:c /home/c
passwd c

# uncomment:
# %wheel ALL=(ALL:ALL) NOPASSWD: ALL 

nano /etc/sudoers

# add user 'c' to group 'wheel'
gpasswd -a c wheel
