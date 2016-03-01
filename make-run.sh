. ./make-env.sh 
echo " linux: $L"
echo "kernel: $I"
echo " ramfs: $F"
echo

#kvm -kernel $L/$I -initrd $F
#kvm -kernel $L/$I -initrd $F -net nic -net user
kvm -kernel $L/$I -initrd $F -netdev user,id=user.0 -device e1000,netdev=user.0
#kvm -kernel $L/$I -initrd $F -netdev user,id=vn -device e1000,netdev=user,id=vn,hostfwd=tcp::5555-:8088
