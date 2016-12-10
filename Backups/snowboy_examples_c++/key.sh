#!/bin/sh
# Activate firefox and press command line arg key
# Examples
# sh key.sh c
# sh key.sh Left

wid=`xdotool search "Mozilla Firefox"`
xdotool windowactivate $wid
sleep 0.2
xdotool key $1
