export OSCA=$HOME/w/osca
export PATH=$HOME/bin:$OSCA/sh:$PATH

# for faster start of xterm
export SESSION_MANAGER=

# set background
xsetroot -bitmap $OSCA/bmp/sq.bmp

# launch clonky
$OSCA/clonky/clonky &

# frameless logging to frameless.log'
exec $OSCA/frameless/frameless
