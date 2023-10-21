#!/bin/sh
# notes.
#   change .xinitrc
#   ln -s .xinitrc .xsession
#   systemctl status display-manager
#   sudo systemctl stop gdm3
#   sudo apt install mousepad thunar libinput-tools
# touchpad
# xinput : to list devices
# xinput list-props [id]
# look for "Tapping Enabled"
xinput set-prop 10 305 1

# look for "Natural Scrolling Enabled"
xinput set-prop 10 284 1

# use gnome-keyring-daemon to manage keys
/usr/bin/gnome-keyring-daemon --start --components=gpg
/usr/bin/gnome-keyring-daemon --start --components=ssh

# visual code
#   when pushing to github have the default browser open for the
#   interaction between visual code and github to work
