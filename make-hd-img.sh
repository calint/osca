###
# howto make bootable hard drive image
#
rm -rf hd.img &&
dd if=/dev/zero bs=1M count=1K | pv | dd of=hd.img &&
parted hd.img mklabel msdos &&
parted hd.img mkpart primary ext4 1M 100% &&
parted hd.img set 1 boot on &&
#sudo umount /dev/loop0p1 &&
sudo losetup -P /dev/loop0 hd.img &&
sudo mkfs.ext4 /dev/loop0p1 &&
. ./make-env.sh && 
if [ -z $T ];then echo " !!! T is not set" && exit 1;fi &&
echo " *** rootfs mounted on $T" &&
rm -rf $T && mkdir $T &&
sudo mount /dev/loop0p1 $T &&
sudo chown -R $(whoami).$(whoami) $T &&
. ./make-rootfs.sh &&
sudo mknod $T/dev/console c 5 1 &&
sudo mknod -m 664 $T/dev/null c 1 3 &&
sudo mknod -m 660 $T/dev/sda b 8 0 &&
sudo mknod -m 660 $T/dev/sda1 b 8 1 &&
sudo extlinux --install $T &&
sudo sudo dd if=/usr/lib/syslinux/bios/mbr.bin of=/dev/loop0 &&
cp -a $L/$I $T/vmlinuz &&
echo "DEFAULT xiinux">>$T/extlinux.conf&&
echo "    SAY booting xiinux osca">>$T/extlinux.conf&&
echo "LABEL xiinux">>$T/extlinux.conf&&
echo "        KERNEL vmlinuz">>$T/extlinux.conf&&
echo "        APPEND root=/dev/sda1 rw init=/init quiet splash">>$T/extlinux.conf&&
sudo chown -R root.root $T/. ||  # expected error 
sudo umount /dev/loop0p1 &&
sudo losetup -D &&
qemu-system-x86_64 -hda hd.img -redir tcp:8088::8088

# sudo mount -o loop,offset=$((2048*512)) hd.raw mnt_hd/