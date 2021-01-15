#!/bin/sh
cat << EOF >> /etc/modules.conf
alias char-major-116 snd
alias snd-card-0 snd-dummy
EOF
cat << EOF > .asoundrc
pcm.dummy {
	type hw
	card 0
}
ctl.dummy {
	type hw
	card 0
}
EOF
apt-get install linux-modules-extra-$(uname -r)
modprobe snd-dummy
usermod -a -G audio travis
alsa-info --stdout --with-alsactl --with-amixer --with-aplay --with-configs --with-devices --with-dmesg
