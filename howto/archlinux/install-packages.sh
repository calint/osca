#!/bin/bash
set -e
cd $(dirname "$0")

packages=$(<packages-common.txt)
yay -S --needed --noconfirm ${packages[@]}
