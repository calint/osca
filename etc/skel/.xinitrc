# path to osca
export OSCA="$HOME/w/osca"

# executables path
export PATH="$HOME/bin:$OSCA/sh:$PATH"

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
