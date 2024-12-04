# notes
* install using profile Xorg
* run install-packages.sh
* install yay
* install brave browser: `yay -S brave-bin`
* `git clone http://github.com/calint/osca` in ~/w/
* `cd ~/w/osca && ./make-all.sh && ./init-user-home.sh`
* logout login
* run: `start-frameless`
* open terminal: Key + c
* 'xinput' to find touchpad, 'xinput list-props x' to find properties:
  - "Tapping Enabled"
  - "Natural Scrolling Enabled"
* 'xinput set-prop x y 1'
  save in script and run at login
* example for HP Stream:
```
#/bin/sh
xinput set-prop 9 306 1
xinput set-prop 9 314 1
```