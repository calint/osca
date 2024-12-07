export OSCA="$HOME/w/osca"
export PATH="$HOME/bin:$OSCA/sh:$PATH"

export PS1=" :: "

# launch frameleses window manager
alias start-frameless='xinit -- :1 -nolisten tcp vt$XDG_VTNR'
# find
alias f='grep --color=always -i'
# list files
alias l='ls --color -lhF'
# list all files (including dot files .*)
alias ll='ls --color -lAF'
# list all file sort on time falling
alias lt='ls --color -lAtF'
# print
alias p='cat'
# ps in 2-columns, id and name
alias psa=$'ps ax|awk \'{print $1 "\t" $5}\''

# archlinux package management
# install package
alias pki='sudo pacman -S'
# list installed packages
alias pkl='pacman -Q'
# query packages
alias pkq='pacman -Ss'
# remove package
alias pkr='sudo pacman -Rsn'
# update packages
alias pku='sudo pacman -Syu'
# list files in package
alias pkf='pacman -Ql'
# find which package a file is in
alias pkfq='pacman -F'
# updates the database of mapping file to package
alias pkfu='sudo pacman -Fy'
