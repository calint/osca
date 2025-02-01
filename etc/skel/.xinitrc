# osca home
export OSCA="$HOME/w/osca"

# add to path if not already there
if [[ ":$PATH:" != *":$OSCA/sh:"* ]]; then
    export PATH="$OSCA/sh:$PATH"
fi

if [[ ":$PATH:" != *":$HOME/bin:"* ]]; then
    export PATH="$HOME/bin:$PATH"
fi

# no session manager
export SESSION_MANAGER=

# turn off 'caps lock'
setxkbmap -option caps:none

# set background
xsetroot -bitmap "$OSCA/bmp/sq.bmp"

# launch clonky
"$OSCA/clonky/clonky" &

# start frameless
exec "$OSCA/frameless/frameless"
