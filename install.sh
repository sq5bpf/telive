#!/bin/bash
T=/tetra
if [ ! -d $T ]; then
	echo "please make a $T directory, and make it writeable as user `whoami`"
	exit
fi
mkdir $T/in $T/out $T/log $T/tmp $T/bin
cp bin/* $T/bin
touch $T/log/telive.log
