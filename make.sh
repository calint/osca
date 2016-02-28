L=linux-4.4.3 &&
B=busybox-1.24.1 &&
CONFIG_LINUX=linux-configs/1.config &&
CONFIG_BUSYBOX=busybox-configs/0.config &&
I=arch/x86/boot/bzImage &&

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

rm -rf $T&&
mkdir -p $T/sbin&&
mkdir -p $T/bin&&
$CC  -o $BIN $SRC $DBG $LIB $OPTS $WARNINGS&& 
echo&&ls -ho --color $BIN&&echo&&

cd $B&&make&&
ls -ho --color busybox&&echo&&
cp -a busybox ../$T/bin&&

cd ../$T&&

cd bin&&
ln busybox sh&&
ln busybox ls&&
ln busybox mount&&
ln busybox hostname&&
ln busybox ifconfig&&
cd ..&&

mkdir proc sys dev &&

echo "#!/bin/sh -x">init&&
echo "mount -t proc proc /proc&&">>init&&
echo "mount -t sysfs sysfs /sys&&">>init&&
echo "ifconfig lo up&&">>init&&
echo "ifconfig eth0 up&&/bin/busybox udhcpc&&">>init&&
echo "ifconfig&&">>init&&
echo "exec /bin/sh">>init&&
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

qemu-system-x86_64 -kernel $L/$I -initrd $F -redir tcp:5555::80

#sudo /etc/qemu-ifup tap0 &&
#qemu-system-x86_64 -kernel $L/$I -initrd $F -net nic -net tap,ifname=tap0,script=no,downscript=no &&
#sudo /etc/qemu-ifdown tap0


