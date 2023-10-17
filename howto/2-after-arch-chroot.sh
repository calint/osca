#!/bin/sh
set -e
cd $(dirname "$0")

pacman -S vi nano git sudo

# create user 'c' and home directory
mkdir /home/c
useradd -d /home/c c
chown -R c:c /home/c
passwd c

# uncomment:
# %wheel ALL=(ALL) NOPASSWD: ALL 

nano /etc/sudoers

gpasswd -a c wheel
