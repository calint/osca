export WORKSPACE=$HOME/w
export OSCA=$WORKSPACE/osca
export IDE=$HOME/downloads/eclipse/eclipse
export XC='xterm -fa mono:size=11 -bg black -fg gray'
export XF=thunar
export XE=leafpad
export XM=vlc
export XV='xterm -e alsamixer'
export XI=chromium
export XX="$OSCA/stickyo/stickyo"
export XQ="$OSCA/menuq/menuq"
export XQQ='$OSCA/menuq/menuq'
export PATH=$PATH:$OSCA/sh
#export JRE=$HOME/downloads/jdk
#export PATH=$JRE/bin:$PATH
export PS1=' :: '

# timezone
export TZ=Asia/Bangkok

alias f='grep --color=always -i'
alias l='ls --color -lhF'
alias ll='ls --color -lAF'
alias lt='ls --color -lAtF'
alias p='cat'
alias pki='sudo pacman -S'
alias pkl='pacman -Q'
alias pkq='pacman -Ss'
alias pkfq='pkgfile'
alias pkfu='sudo pkgfile -u'
alias pkr='sudo pacman -Rsn'
alias pku='sudo pacman -Syu'
alias x-ide='~/downloads/eclipse/eclipse'
alias pkf='pkgfile -l'
alias get-kernel-module-params='systool -vm'
alias start-frameless='xinit -- :1 -nolisten tcp vt$XDG_VTNR'
alias psa=$'ps ax|awk \'{print $1 "\t" $5}\''