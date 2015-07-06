#!/bin/bash
# install_telive.sh (c) 2015 Jacek Lipkowski <sq5bpf@lipkowski.org>
#
# simple script to install telive under Debian 8
# this is a quick hack, with bad error checking etc.
# some day i will make a proper install script, but for now this will have to do
#
# This script is licensed under GPL v2
#
# I disclaim any liability for things that this software does or doesn't do.
# Everything is the responsibility of the user.
#
# Changelog:
# 20150706: forbid running script as root --sq5bpf
#

TETRADIR=~/tetra
verify_prerequisites() {
	echo "CHECKING prerequisites"
	#verify distribution
	MAJVER=`cut -d . -f 1 /etc/debian_version`
	#comment this out if you know what you're doing :)
	if [ "$MAJVER" != 8 ]; then
		echo "This will only work under debian 8, aborting install..."
		exit 1 #comment out this line if you want to install on another distribution
	fi

	#check if running as root
	if [ `id -u` = 0 ]; then
		echo "You are running this as root. Please run as a normal user, and configure sudo for this user"
		exit 1
	fi

	#check sudo
	echo "Checking sudo..."
	which sudo >/dev/null ; RET=$?
	if [ "$RET" != 0 ]; then
		echo "You don't have sudo installed, please install sudo, and configure it so that you can run root commands as your regular user"
		echo "As root run this:"
		echo "apt-get -y install sudo"
		echo "echo \"`whoami`	ALL=(ALL:ALL) ALL\" >> /etc/sudoers"
		exit 2
	fi

	echo "Now trying sudo, please provide your password if asked"
	ROOTID=`sudo id -un`
	if [ "$ROOTID" != "root" ]; then
		echo "sudo is installed, but not configured correctly, or you gave it the wrong password"
		echo "please fix it, and re-run this script"
		exit 3
	fi
}

install_packages() {
	echo "INSTALLING packages"
	sudo apt-get update

	GR_VERSION=`gnuradio-config-info -v 2>/dev/null|tr -d v`

	if [ "$GR_VERSION" ]; then
		echo "Gnuradio $GR_VERSION seems to be already installed, so not installing it"
	else
		sudo apt-get -y install gnuradio gnuradio-dev gr-osmosdr gr-iqbal
	fi

	sudo apt-get -y install git make libtool libncurses5-dev build-essential autoconf automake vorbis-tools sox alsa-utils unzip xterm


}

install_codec () {
	echo "INSTALLING codec"
	git clone https://github.com/sq5bpf/install-tetra-codec && \
		cd install-tetra-codec && \
		chmod 755 install.sh && \
		./install.sh
}

install_libosmocore () {
	echo "INSTALLING libosmocore"
	git clone https://github.com/sq5bpf/libosmocore-sq5bpf && \
		cd libosmocore-sq5bpf &&  \
		autoreconf -i && \
		./configure && \
		make && \
		sudo make install && \
		sudo ldconfig
}

install_osmo_tetra_sq5bpf() {
	echo "INSTALLING osmo-tetra-sq5bpf"
	git clone https://github.com/sq5bpf/osmo-tetra-sq5bpf && \
		cd osmo-tetra-sq5bpf/src && \
		make
}

install_telive() {
	echo "INSTALLING telive"
	git clone https://github.com/sq5bpf/telive && \
		cd telive && \
		make && \
		chmod 755 install.sh && \
		./install.sh
}

######## MAIN
echo "Telive simple installer"

verify_prerequisites

#if there is a previous install then kill it
rm -fr "$TETRADIR" >/dev/null 2>/dev/null 

mkdir -p $TETRADIR
cd "$TETRADIR" || exit 1

echo "Please make sure that you have full internet access"
( install_packages || exit 1 )
( install_codec || exit 1)
if [ -d "/tetra" ]; then
	:
else    
	echo "You're missing the /tetra directory, probably something wrong with the codec installation"	
	exit 1
fi
( install_libosmocore || exit 1 )
( install_osmo_tetra_sq5bpf || exit 1 )
( install_telive || exit 1)

#please note that we should actually check if everything went correctly, 
#not just assume it :)
echo; echo
echo "It seems that everything installed correctly :)"
echo "All of the files are in `pwd`"
echo
echo "PLEASE, before proceeding read the manual in `pwd`/telive/telive_doc.pdf"
