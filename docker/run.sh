#!/usr/bin/env bash


# echo "load-module module-esound-protocol-tcp auth-anonymous=1" >> /etc/pulse/system.pa


if [ ! -f files/adduser.sh ]; then
    # prepare some black magic
    echo $USER > files/user
    cat > files/adduser.sh << EOF
adduser --disabled-password --gecos '' --uid $(id -u $USER) $USER
EOF
    chmod +x files/adduser.sh
fi
docker build -t telive .

echo "starting..."
docker run -e  DISPLAY=$DISPLAY --device /dev/snd -v /tmp/.X11-unix:/tmp/.X11-unix --rm -u $USER --net=host -v `pwd`:/tmp/current  --privileged -v /dev/bus/usb:/dev/bus/usb  -e PULSE_SERVER=tcp:$(hostname -i):4713 -e PULSE_COOKIE=/run/pulse/cookie -v ~/.config/pulse/cookie:/run/pulse/cookie -u $USER -it telive /tmp/run.sh