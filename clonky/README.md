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

* change layout by editing `main.c` function `render`
