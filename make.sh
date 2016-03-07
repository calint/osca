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

CC="g++ -std=c++11" &&
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

rm -rf $T &&
mkdir -p $T/bin &&
$CC -o $BIN $SRC $DBG $LIB $OPTS $WARNINGS && 
ls -ho --color $BIN &&

cd $B &&
make &&
cd .. &&
cp -a $B/busybox $T/bin &&
ls -ho --color $T/bin/busybox &&

cd $T/bin &&
ln busybox sh &&
ln busybox ash &&
ln busybox ls &&
ln busybox mount &&
ln busybox hostname &&
ln busybox ifconfig &&
ln busybox route &&
ln busybox nameserver &&
ln busybox ip &&
ln busybox udhcpc &&
#ln busybox cat &&
#ln busybox ping &&
cd .. &&

mkdir proc sys dev etc &&

cp -a ../$S/udhcpc.script etc && chmod +x etc/udhcpc.script &&

cat<<EOF>init
#!/bin/sh
mount -t proc proc /proc &&
mount -t sysfs sysfs /sys &&
ip link set lo up &&
ip link set eth0 up && udhcpc -s /etc/udhcpc.script &&
ip addr show &&
#exec busybox ash
exec xiinux p
EOF

chmod +x init&&

fakeroot sh -c "mknod dev/console c 5 1 && mknod -m 664 dev/null c 1 3 && chown -R root.root . && find .|cpio --quiet -H newc -o > ../$F" &&
cd .. &&
ls -ho --color $F && echo &&

echo " * building kernel"&&
cd $L &&
make -j2 &&
echo && ls -ho --color $I && echo &&
cd .. &&

. ./make-run.sh
