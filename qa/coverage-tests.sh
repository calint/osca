port=8088
#port=80
server=localhost
#server=www.ramvark.net
host=$server:$port
#sessionid=aaaa-110307-064915.110-cb48e04f
#cookie="i=$sessionid"
#uploadfile="logo.jpg"
#uploadfile_verysmall="verysmall.txt"
#uploaddir="upload dir"
#classpath="/Users/calin/Documents/workspace/a/bin"

alias p='echo -n'
alias pl=echo

pl $(date) &&
pl "$0" &&
pl $host &&
#echo $sessionid
#echo $classpath
pl &&
p t001: &&
curl -s http://$host/ > file &&
diff -q file t001.cmp &&
rm file &&
pl ok &&

p t002: &&
curl -s http://$host/?hello > file &&
diff -q file t002.cmp &&
rm file &&
pl ok &&
pl&&
pl $(date)