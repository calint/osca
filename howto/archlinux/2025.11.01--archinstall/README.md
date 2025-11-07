# notes

## installation

- connect wifi using `https://wiki.archlinux.org/title/Iwd`
- `archinstall` using profile `Xorg` with `pipewire` sound
- run `install-step-1.sh`
- run `install-step-2.sh`
- install `yay`:

```sh
git clone https://aur.archlinux.org/yay.git
cd yay
makepkg -si
yay --version
```

- install brave browser: `yay -S brave-bin`
- logout login
- run: `start-frameless`
- to configure touchpad see `config-touchpad.sh` or:
  - open terminal: `Super + c`
  - `xinput` to find touchpad, `xinput list-props x` to find properties:
    - "Tapping Enabled"
    - "Natural Scrolling Enabled"
  - `xinput set-prop x y 1`
    save in script and run at login
  - example for HP Stream Notebook PC 11:

    ```sh
    #/bin/sh
    xinput set-prop 9 306 1
    xinput set-prop 9 314 1
    ```

## configure bluetooth speaker

enable bluetooth daemon

```sh
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

## on laptop

- install power saving service

```sh
sudo pacman -S tlp
sudo systemctl enable tlp
sudo systemctl start tlp
```

- install `powertop` for overview of power consumption

```sh
sudo pacman -S powertop
```

## troubleshooting

### ASUS Zenbook 14 UM3406KA-PP761WA

- at reboot `F2` for BIOS, `Esc` for boot menu
- disable "secure boot" in bios to boot archlinux iso
- if `clonky` hangs make sure `bluetooth` service is enabled and command
  `bluetoothctl devices Connected` works
- if experiencing lag on input to `xterm`, or lag in `clonky` or other X11
  applications, then add kernel parameter `amdgpu.dcdebugmask=0x10` in boot loader
  entry. **note: `kitty` does not exhibit this problem, running lts kernel also
  fixes it**
- in `/etc/tlp.conf` uncomment `START_CHARGE_THRESH_BAT0=75` and
  `STOP_CHARGE_TRESH_BAT0=80` for better long term battery health
