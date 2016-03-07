. ./make-env.sh

echo "         linux: $L"
echo "  config linux: $CONFIG_LINUX"
echo "config busybox: $CONFIG_BUSYBOX"
echo "        kernel: $I"
echo "         ramfs: $F"
echo

cp -a $CONFIG_LINUX $L/.config &&
cp -a $CONFIG_BUSYBOX $B/.config &&

T=rootfs
S=src

[ -d "$T" ] && rm -rf $T &&
mkdir $T && cd $T && mkdir bin proc sys dev etc && cd .. &&

# make xiinux
CC='g++ -std=c++11' &&
BIN=$T/bin/xiinux &&
SRC=$S/xiinux.cpp &&
DBG= &&
#DBG="-g -O0" &&
#DBG="$DBG --coverage -fprofile-arcs -ftest-coverage" &&
#OPTS=-Os &&
OPTS="-O3 -static" &&
WARNINGS="-Wall -Wextra -Wpedantic -Wno-unused-parameter -Wfatal-errors" &&
#LIB="-pthread -lgcov" &&
LIB=-pthread &&
#LIB= &&

$CC -o $BIN $SRC $DBG $LIB $OPTS $WARNINGS && 
ls -ho --color $BIN &&

# make busybox
cd $B && make -j2 && cd .. && cp -a $B/busybox $T/bin &&
cd $T && ls -ho --color bin/busybox &&

###################################################################################################
cat<<'EOF'>init
#!/bin/busybox ash
echo && echo &&
echo '- -  - -- - -  - - --- - - - - - - - - - - - - - -- - - ' &&
echo '-- - xiinux  - --- -- - - - -- - - -- - - - - -- - - - -' &&
echo '- - - - - - - ---- -- - - - - -- - - - - -- - - - - - - ' &&
busybox mount -t proc proc /proc &&
busybox mount -t sysfs sysfs /sys &&
busybox ip link set lo up &&
busybox ip link set eth0 up && busybox udhcpc -s /etc/udhcpc.script && echo &&
busybox ip addr show && echo &&
busybox free && echo &&
busybox ps && echo &&
exec xiinux
#exec xiinux -p
#exec busybox ash
EOF
chmod +x init &&
###################################################################################################
cat<<'EOF'>etc/udhcpc.script
#!/bin/busybox ash
# udhcpc script edited by Tim Riker <Tim@Rikers.org>
# modifications by Mattias Schlenker <ms@mattiasschlenker.de>
# modifications by Calin Tenitchi <calin.tenitchi@gmail.com>
#   * adjusted for linkless busybox install

echo "*** DHCP event '$1'"
echo "*** ifc: $interface   ip: $ip   bcast: $broadcast   subnet: $subnet"
echo "*** router: $router   dns: $dns   domain: $domain"

### !!! get time using ntp

RESOLV_CONF=/etc/resolv.conf

[ -z "$1" ] && echo "error: should be called from udhcpc" && exit 1
[ -n "$broadcast" ] && BROADCAST="broadcast $broadcast"
[ -n "$subnet" ] && NETMASK="netmask $subnet"

case "$1" in 
        deconfig)
                busybox ifconfig $interface 0.0.0.0
                ;;
        renew|bound)
                busybox ifconfig $interface $ip $BROADCAST $NETMASK
                if [ -n "$router" ] ; then
#                        echo "*** deleting routers"
                        while busybox route del default gw 0.0.0.0 dev $interface 2>/dev/null; do
                                :
                        done
                        for i in $router ; do
#                                echo "*** adding default route $i"
                                busybox route add default gw $i dev $interface
                        done
                fi

                echo -n > $RESOLV_CONF
		[ -n "$dns" ] && echo -n >$RESOLV_CONF
                [ -n "$domain" ] && echo search $domain >> $RESOLV_CONF
		for i in $dns ; do
#                        echo "*** adding dns $i"
                        echo nameserver $i >> $RESOLV_CONF
                done
                ;;
esac
#echo "***"
exit 0
EOF
###################################################################################################
chmod +x etc/udhcpc.script &&

#find && echo &&
# make initramfs
fakeroot sh -c "mknod dev/console c 5 1 && mknod -m 664 dev/null c 1 3 && chown -R root.root . && find .|cpio --quiet -H newc -o > ../$F" &&
cd .. &&
ls -ho --color $F &&

echo " * building kernel"&&
cd $L &&
make -j2 &&
echo && ls -ho --color $I && echo &&
cd .. &&

. ./make-run.sh
