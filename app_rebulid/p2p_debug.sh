#!/bin/sh

while [ 1 ]
do
	#/usr/share/ipcam/shell/IOTDaemon  -S "JA$1" -L /media/tf/
	/usr/share/ipcam/shell/IOTDaemon  -S "JA$1"
	sleep 1
done

