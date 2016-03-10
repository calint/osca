. ./make-env.sh 
alias p=echo
p "  linux: $L"
p " kernel: $I"
p "  ramfs: $F"
p
set -x
qemu-system-x86_64 -enable-kvm -kernel $L/$I -initrd $F -redir tcp:8088::8088 -nographic -append 'quiet console=ttyS0'
#qemu-system-x86_64 -enable-kvm -nographic -kernel $L/$I -append 'root=/dev/sda1 init=init rw console=ttyS0' -hda hd.raw -redir tcp:8088::8088
#qemu-system-x86_64 -enable-kvm -hda hd.raw -redir tcp:8088::8088
#qemu-system-x86_64 -enable-kvm -kernel $L/$I -initrd $F -redir tcp:8088::8088 -nographic -append 'console=ttyS0'
#qemu-system-x86_64 -enable-kvm -hda hd.raw -kernel $L/$I -initrd $F -redir tcp:8088::8088 -nographic -append 'quiet console=ttyS0'
#qemu-system-x86_64 -enable-kvm -kernel $L/$I -initrd $F -redir tcp:8088::8088
