xsetroot -bitmap $OSCA/bmp/sq.bmp
setxkbmap -option caps:none
synclient VertTwoFingerScroll=1 VertScrollDelta=-111 HorizTwoFingerScroll=1 HorizScrollDelta=-111 TapButton1=1 TapButton2=3 TapButton3=2 PalmDetect=1
#exec xterm
#xterm & exec $OSCA/frameless/frameless
$OSCA/clonky/clonky & exec $OSCA/frameless/frameless
