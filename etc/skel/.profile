export WORKSPACE=$HOME/w
export OSCA=$WORKSPACE/osca
export PATH=$PATH:$OSCA/sh
export PS1=' :: '

alias start-frameless='xinit -- :1 -nolisten tcp vt$XDG_VTNR'
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
alias psa=$'ps ax|awk \'{print $1 "\t" $5}\''
