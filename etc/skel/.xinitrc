# set background
xsetroot -bitmap $OSCA/bmp/sq.bmp

# launch clonky
$OSCA/clonky/clonky &

# frameless logging to frameless.log'
exec $OSCA/frameless/frameless
