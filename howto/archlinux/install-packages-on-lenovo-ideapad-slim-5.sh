#!/bin/bash
packages=(
7zip
acpi
alsa-utils
base
base-devel
bash-completion
blender
bluez
bluez-utils
brave-bin
brightnessctl
ca-certificates-mozilla
clang
cmake
cutecom
dosfstools
efibootmgr
eog
exfat-utils
feh
ffmpegthumbnailer
gimp
git
glm
gnu-netcat
gtk4
gtkwave
gvfs
gvfs-mtp
htop
intel-oneapi-tbb
inxi
iverilog
iwd
jq
lcov
less
libgepub
libgsf
libopenraw
libva-intel-driver
libva-utils
libxcrypt-compat
libxft
linux
linux-firmware
llvm
man-db
mariadb
mesa-utils
mousepad
mpv
nano
nasm
ncurses5-compat-libs
openfpgaloader
pacman-contrib
pavucontrol
perf
pipewire
pipewire-alsa
pipewire-jack
pipewire-pulse
plocate
poppler-glib
powertop
putty
qemu-system-x86
qemu-ui-gtk
riscv64-elf-gcc
rsync
scrot
sdl2-compat
sdl2_image
sdl2_ttf
sysstat
tcl
thunar
tiled
tinyxxd
tk
tlp
ttf-dejavu
ttf-jetbrains-mono
tumbler
valgrind
verible-bin
visual-studio-code-bin
wireplumber
xclip
xorg-server
xorg-xev
xorg-xinit
xorg-xinput
xorg-xsetroot
xterm
yay
zram-generator
)
yay -S --needed "${packages[@]}"
