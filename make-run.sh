. ./make-env.sh 
echo " linux: $L"
echo "kernel: $I"
echo " ramfs: $F"
echo

qemu-system-x86_64 -enable-kvm -kernel $L/$I -initrd $F -redir tcp:8088::8088 -nographic -append 'console=ttyS0'
#qemu-system-x86_64 -enable-kvm -kernel $L/$I -initrd $F -redir tcp:8088::8088
