. ./make-env.sh &&
cp -a $L/$I linux-kernels/$1-linux &&
cp -a $L/.config linux-configs/$1-config
ls -la linux-kernels/$1-linux &&
ls -la linux-configs/$1-config
