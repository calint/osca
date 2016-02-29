. ./make-env.sh

CONFIG_LINUX=linux-configs/1.config &&
CONFIG_BUSYBOX=busybox-configs/0.config &&

cp -a $CONFIG_LINUX $L/.config &&
cp -a $CONFIG_BUSYBOX $B/.config &&

T=rootfs &&
F=fs.cpio &&
S=src &&

#CC="clang++ -std=c++11" &&
CC="g++ -std=c++11" &&
#BIN=xiinux &&
BIN=$T/sbin/xiinux &&
SRC=$S/xiinux.cpp &&
DBG= &&
#DBG="-g -O0" &&
#DBG="$DBG --coverage -fprofile-arcs -ftest-coverage" &&
#OPTS=-Os &&
OPTS="-O3 -static" &&
WARNINGS="-Wall -Wextra -Wpedantic -Wno-unused-parameter -Wfatal-errors" &&
#LIB="-pthread -lgcov" &&
LIB=-pthread &&

rm -rf $T &&
mkdir -p $T/sbin &&
mkdir -p $T/bin &&
$CC  -o $BIN $SRC $DBG $LIB $OPTS $WARNINGS && 
echo && ls -ho --color $BIN && echo &&

cd $B &&
make &&
ls -ho --color busybox&&echo&&
cp -a busybox ../$T/bin&&

cd ../$T &&

cd bin &&
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
ln busybox cat &&
ln busybox ping &&
cd .. &&

mkdir proc sys dev etc &&

cp -a ../$S/udhcpc.script etc/ && chmod +x etc/udhcpc.script &&

cat<<EOF>init
#!/bin/sh -x
mount -t proc proc /proc &&
mount -t sysfs sysfs /sys &&
ip link set lo up &&
ip link set eth0 up && udhcpc -s /etc/udhcpc.script &&
ip addr show &&
exec /bin/sh
EOF
#echo "exec /sbin/xiinux">>init&&
chmod +x init&&

# mknod -m 666 dev/tty c 5 0 
fakeroot sh -c "mknod dev/console c 5 1 && mknod -m 664 dev/null c 1 3 && chown -R root.root . && find .|cpio -H newc -o > ../$F" &&
cd .. &&
echo && ls -ho --color fs.cpio && echo &&

echo " * building kernel"&&
cd $L &&
make &&
echo && ls -ho --color $I && echo &&
cd .. &&

# ! guest closes connection
qemu-system-x86_64 -append "quiet" -kernel $L/$I -initrd $F -redir tcp:5555::80 

# ! guest behind firewall, cannot web serve
#qemu-system-x86_64 -append "quiet" -kernel $L/$I -initrd $F -net nic -net user
#qemu-system-x86_64 -kernel $L/$I -initrd $F -net nic -net user -nographic -curses
#qemu-system-x86_64 -kernel $L/$I -initrd $F -net nic -net user

# ! does not work on mine
#sudo /etc/qemu-ifup tap0 &&
#qemu-system-x86_64 -kernel $L/$I -initrd $F -net nic -net tap,ifname=tap0,script=no,downscript=no &&
#sudo /etc/qemu-ifdown tap0


