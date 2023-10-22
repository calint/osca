export OSCA=$HOME/w/osca
export PATH=$HOME/bin:$OSCA/sh:$PATH

# for faster start of xterm
export SESSION_MANAGER=

# turn 'caps lock' into 'shift'
setxkbmap -option caps:shift

# set background
xsetroot -bitmap $OSCA/bmp/sq.bmp

# launch clonky
$OSCA/clonky/clonky &

# start frameless
exec $OSCA/frameless/frameless
