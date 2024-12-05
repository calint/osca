#!/bin/sh
set -e

sudo apt update
sudo apt upgrade
sudo apt install git
mkdir w
cd w
git clone https://github.com/calint/osca
