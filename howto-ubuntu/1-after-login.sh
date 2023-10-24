#!/bin/sh
# enabling settings on touchpad
# 'xinput' to list devices
# 'xinput list-props [id]' to list properties

# look for "Tapping Enabled"
xinput set-prop 10 305 1

# look for "Natural Scrolling Enabled"
xinput set-prop 10 284 1

# use gnome-keyring-daemon to manage keys
#/usr/bin/gnome-keyring-daemon --start --components=gpg
#/usr/bin/gnome-keyring-daemon --start --components=ssh

# visual code
#   when pushing to github have the default browser open for the
#   interaction between visual code and github to work
