# notes
* on HP Stream 11: at iwctl power off and on the device for scan to work
* install using profile Xorg
* run `install-packages.sh`
* install yay
```
git clone https://aur.archlinux.org/yay.git
cd yay
makepkg -si
yay --version
```
* install brave browser: `yay -S brave-bin`
* `git clone http://github.com/calint/osca` in `~/w/`
* `cd ~/w/osca && ./make-all.sh && ./init-user-home.sh`
* logout login
* run: `start-frameless`
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