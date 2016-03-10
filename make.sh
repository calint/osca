. ./make-env.sh &&
rm -rf $T &&
. ./make-rootfs.sh &&
# make initramfs
cd $T &&
fakeroot sh -c "mknod dev/console c 5 1 && mknod -m 664 dev/null c 1 3 && mknod -m 660 dev/sda b 8 0 && mknod -m 660 dev/sda1 b 8 1 && chown -R root.root . && find .|cpio --quiet -H newc -o > ../$F" &&
#fakeroot sh -c "mknod dev/console c 5 1 && mknod -m 664 dev/null c 1 3 && chown -R root.root . && find .|cpio --quiet -H newc -o > ../$F" &&
cd .. &&
ls -ho --color $F &&

echo " * building kernel"&&
BACK_PWD=$(pwd)
cd $L &&
make -j2 &&
echo && ls -ho --color $I &&
echo &&
cd $BACK_PWD &&

. ./make-run.sh
