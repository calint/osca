# notes
* on HP Stream Notebook PC 11 at install at iwctl power off and on the device for scan to work
* install using profile Xorg with pipewire sound
* run `install-step-1.sh`
* run `install-step-2.sh`
* install `yay`
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
* example for: HP Stream Notebook PC 11
```
#/bin/sh
xinput set-prop 9 306 1
xinput set-prop 9 314 1
```

## configure bluetooth speaker
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