# dependencies

## uses shell commands

* `acpi`
* `journalctl`
* `df`
* `iostat`
* `iw`
* `bluetoothctl`

## uses information from

* `/proc/meminfo`
* `/proc/stat`
* `/proc/swaps`
* `/sys/class/power_supply/`
* `/sys/class/net/.../statistics/`
* `/sys/devices/system/cpu/`

## howto

* modify `main-cfg.h` for settings and `main.c` function `render` for layout
