notes on enabling frameless in ubuntu 23.10

* if thunar, mousepad and firefox startup takes 10-20 seconds then a timeout
  is generated and can be view in /var/log/syslog
  fix is:
    systemctl --user mask xdg-desktop-portal-gnome
    systemctl --user stop xdg-desktop-portal-gnome

    :: ll .config/systemd/user/
    total 0
    lrwxrwxrwx 1 c c 9 nov  6 13:13 ubuntu-report.service -> /dev/null
    lrwxrwxrwx 1 c c 9 nov  6 11:21 xdg-desktop-portal-gnome.service -> /dev/null

* to set up user home see 'init-user-home.sh'

* to enable option on login screen add file: /usr/share/xsessions/xsession.desktop
    --------------------------
    [Desktop Entry]
    Name=XSession
    Exec=/etc/X11/Xsession
    --------------------------
    this will launch '~/.xsession' at login

* to enable tap and natural scrolling see '1-after-login.sh'

* press Fn + Esc will toggle between enabling / disabling Fn-key functions.

* additional packages:
    sudo apt install mousepad thunar scrot libinput-tools xbacklight
