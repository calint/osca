# osca home
export OSCA="$HOME/w/osca"

# add to path if not already there
if [[ ":$PATH:" != *":$OSCA/sh:"* ]]; then
    export PATH="$OSCA/sh:$PATH"
fi

if [[ ":$PATH:" != *":$HOME/bin:"* ]]; then
    export PATH="$HOME/bin:$PATH"
fi

# enables file upload in brave
dbus-update-activation-environment --systemd DISPLAY XAUTHORITY

# load default settings
xrdb -load ~/.Xresources

# re-map 'caps lock'
setxkbmap -option ctrl:nocaps

# set background
xsetroot -bitmap "$OSCA/bmp/sq24.bmp"

# launch clonky
"$OSCA/clonky/clonky" &

# start frameless
exec "$OSCA/frameless/frameless"
