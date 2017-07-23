#!/bin/bash

if [ ! -d /sys/class/gpio/gpio157 ]
then
	echo 157 > /sys/class/gpio/export
	sleep 1
fi

echo in > /sys/class/gpio/gpio157/direction

./main c 

exit 0
