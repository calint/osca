# notes
* connect wifi using `https://wiki.archlinux.org/title/Iwd`
* `archinstall` using profile `Xorg` with `pipewire` sound
* run `install-step-1.sh`
* run `install-step-2.sh`
* install `yay`:
```
git clone https://aur.archlinux.org/yay.git
cd yay
makepkg -si
yay --version
```
* install brave browser: `yay -S brave-bin`
* logout login
* run: `start-frameless`
* to configure touchpad see `config-touchpad.sh` or:
* open terminal: `Super + c`
* `xinput` to find touchpad, `xinput list-props x` to find properties:
  - "Tapping Enabled"
  - "Natural Scrolling Enabled"
* `xinput set-prop x y 1`
  save in script and run at login
* example for HP Stream Notebook PC 11:
```
#/bin/sh
xinput set-prop 9 306 1
xinput set-prop 9 314 1
```

## configure bluetooth speaker
enable bluetooth daemon
```
sudo systemctl enable bluetooth
sudo systemctl start bluetooth
```
from `https://wiki.archlinux.org/title/Bluetooth_headset`
```
bluetoothctl
[bluetooth]# power on
[bluetooth]# agent on
[bluetooth]# default-agent
[bluetooth]# scan on
[bluetooth]# pair <device id>
[bluetooth]# connect <device id>
[bluetooth]# trust <device id>
[bluetooth]# scan off
[bluetooth]# exit
```

# troubleshooting

## HP Stream Notebook PC 11

### install media on usb does not boot
* disable secure boot in bios
* legacy mode can be disabled

### at install in `iwctl`: cannot find networks with  `scan`
* power off and on the device
```
device wlan0 set-property Powered off
device wlan0 set-property Powered on
```
### wifi connectivity issues with disconnects
* try `/etc/modprobe.d/rtl8723be.conf`:
```
# disables power saving functions
options rtl8723be aspm=0 ips=0 fwlps=0
```

## Acer Aspire Lite AL14-51M-56HU

### no sound in the speakers
* sudo pacman -S sof-firmware
