#!/bin/sh
set -e

sudo apt update
sudo apt -y upgrade
sudo apt -y install git
mkdir w
cd w
git clone https://github.com/calint/osca
