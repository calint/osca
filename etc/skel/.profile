export OSCA=$HOME/w/osca
export PATH=$PATH:$OSCA/sh

export PS1=' :: '

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
# install package
alias pki='sudo pacman -S'
# list installed packages
alias pkl='pacman -Q'
# query packages
alias pkq='pacman -Ss'
# find which package a file is in
alias pkfq='pkgfile'
# updates the database of mapping file to package
alias pkfu='sudo pkgfile -u'
# remove package
alias pkr='sudo pacman -Rsn'
# update packages
alias pku='sudo pacman -Syu'
# list files in package
alias pkf='pkgfile -l'
# ps in 2-columns, id and name
alias psa=$'ps ax|awk \'{print $1 "\t" $5}\''
