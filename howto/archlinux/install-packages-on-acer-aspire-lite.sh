#!/bin/bash
set -e
cd $(dirname "$0")

packages=$(<packages-acer-aspire-lite.txt)
yay -S --needed --noconfirm "${packages[@]}"
