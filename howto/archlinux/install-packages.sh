#!/bin/bash
set -e
cd $(dirname "$0")

packages=$(<packages.txt)
yay -S --needed --noconfirm "${packages[@]}"
