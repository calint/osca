. ./make-env.sh
#set -x
alias p=echo;alias b=busybox
p
p "         linux: $L"
p "        kernel: $L/$I"
p "  config linux: $CONFIG_LINUX"
cp -a $CONFIG_LINUX $L/.config &&
p "config busybox: $CONFIG_BUSYBOX"
cp -a $CONFIG_BUSYBOX $B/.config &&
p "        rootfs: $T"
p "         ramfs: $F"
p "        source: src"
p

mkdir -p $T/{bin,proc,sys,dev,etc,web} &&

# make xiinux
CC='g++ -std=c++11' &&
BIN=$T/bin/xiinux &&
SRC=src/xiinux.cpp &&
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
cd $B && make -j && cd .. && cp -a $B/busybox $T/bin &&

BACK_PWD=$(pwd) &&
cp -ar web $T/ &&
cd $T &&
ls -ho --color bin/busybox &&
cd bin && ln busybox sh && cd .. &&
###################################################################################################
cat<<'EOF'>init
#!/bin/busybox ash
alias p=echo
alias b=busybox
p && p &&
p '#                                  oOo.o.               '
p '#                                 oOo.oOo               '
p '#      __________________________  .oOo.                '
p '#     O\        -_   .. \      _ \                      '
p '#    O  \                \     \\ \                     '
p '#   o   /\                \    |\\ \                    '
p '#  .   //\\                \   ||   \                   '
p '#   .  \\/\\                \  \_\   \                  '
p '#    .  \\//\________________\________\                 '
p '#     .  \/_/, \\\--\\..\\ - /\_____  /                 '
p '#      .  \ \ . \\\__\\__\\./ / \__/ /                  '
p '#       .  \ \ , \    \\ ///./ ,/./ /                   '
p '#        .  \ \___\              / /                    '
p '#         .  \/\________________/ /                     '
p '#    ./\.  . / /                 /                      '
p '#    /--\   .\/_________________/                       '
p '#         ___.                 .                        '
p '#        |o o|. . . . . . . . .                         '
p '#        /| |\ . .                                      '
p '#    ____       . .                                     '
p '#   |O  O|       . .                                    '
p '#   |_ -_|        . .                                   '
p '#    /||\                                               '
p '#      ___                                              '
p '#     /- -\                                             '
p '#    /\_-_/\                                            '
p '#      | |                       xiinux  osca  linux    '
p '#                                                       '
p '#                                                       '
b uname -a &&
b mount -t proc proc /proc &&
b mount -t sysfs sysfs /sys &&
b ip link set lo up &&
b ip link set eth0 up && b udhcpc -s /etc/udhcpc.script && p &&
b ip addr show && p &&
b free && p &&
p && p &&
cd web && xiinux &
#exec xiinux -p
#exec busybox chroot /web xiinux
exec busybox ash
EOF
chmod +x init &&
###################################################################################################
cat<<'EOF'>etc/udhcpc.script
#!/bin/busybox ash
# udhcpc script edited by Tim Riker <Tim@Rikers.org>
# modifications by Mattias Schlenker <ms@mattiasschlenker.de>
# modifications by Calin Tenitchi <calin.tenitchi@gmail.com>
#   * adjusted for linkless busybox install

[ -z "$1" ]&&p "error: first param is event name from udhcpc"&&exit 1

alias p=echo;alias b=busybox;alias pn='echo -n'

p "*** dhcp says '$1'"
p "***   ifc: $interface   ip: $ip   subnet: $subnet   bcast: $broadcast"
p "***   router: $router   dns: $dns   domain: $domain"

### !!! get time using ntp

case "$1" in 
        deconfig)b ifconfig $interface 0.0.0.0;;
        renew|bound)
		#p ifconfig $interface $ip $([ -n "$subnet" ]&&pn "netmask $subnet") $([ -n "$broadcast" ]&& pn "broadcast $broadcast")
		b ifconfig $interface $ip $([ -n "$subnet" ]&&pn "netmask $subnet") $([ -n "$broadcast" ]&& pn "broadcast $broadcast")
                if [ -n "$router" ];then
                        while b route del default gw 0.0.0.0 dev $interface 2>/dev/null;do :;done
		        for i in "$router";do b route add default gw $i dev $interface;done
                fi
		[ -n "$dns" ]&&p -n>/etc/resolv.conf
                [ -n "$domain" ]&&p search $domain>>/etc/resolv.conf
		for i in "$dns";do p nameserver $i>>/etc/resolv.conf;done
	;;
esac
#p "***"
exit 0
EOF
###################################################################################################
chmod +x etc/udhcpc.script &&
cd $BACK_PWD

