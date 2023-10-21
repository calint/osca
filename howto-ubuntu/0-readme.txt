notes about enabling frameless in ubuntu 23.04

to enable option on login screen add file:
/usr/share/xsessions/xsession.desktop
--------------------------
[Desktop Entry]
Name=XSession
Exec=/etc/X11/Xsession
--------------------------
this will launch '~/.xsession' at login

to enable tap and natural scrolling see '1-after-login.sh'

packages:
sudo apt install mousepad thunar libinput-tools

