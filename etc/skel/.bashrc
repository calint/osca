# osca home
export OSCA="$HOME/w/osca"

# add to path if not already there
if [[ ":$PATH:" != *":$OSCA/sh:"* ]]; then
    export PATH="$OSCA/sh:$PATH"
fi

if [[ ":$PATH:" != *":$HOME/bin:"* ]]; then
    export PATH="$HOME/bin:$PATH"
fi

# prompt
export PS1=" :: "

# displays colored output
export LESS='-R'

# scale Qt UI for high resolution screens
export QT_SCALE_FACTOR=1.25

# launch frameleses window manager
alias start-frameless='exec xinit -- -nolisten tcp vt$XDG_VTNR'
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

# git add, commit, push
alias gcp='git add . && git commit -m "." && git push'
# git tag, push
alias gtp='TAG=$(date "+%Y-%m-%d--%H-%M") && git tag $TAG && git push origin $TAG'
