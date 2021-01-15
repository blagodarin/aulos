#!/bin/sh
cat << EOF > .asoundrc
pcm.!default {
	type hw
	card Dummy
}
EOF
apt-get install linux-modules-extra-$(uname -r)
modprobe snd-dummy
usermod -a -G audio travis
alsa-info --stdout --with-alsactl --with-amixer --with-aplay --with-configs --with-devices --with-dmesg
