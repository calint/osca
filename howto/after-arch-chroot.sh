#!/bin/sh
set -e
cd $(dirname "$0")

pacman -S vi nano git sudo
nano /etc/sudoers

# uncomment:
# %wheel ALL=(ALL) NOPASSWD: ALL 

gpasswd -a c wheel
