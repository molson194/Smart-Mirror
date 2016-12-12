#!/bin/sh
# Activate chromium and press command line arg key
# Examples
# sh key.sh c
# sh key.sh Left

wid=$(xdotool search --onlyvisible --class chromium|head -1)
xdotool windowactivate ${wid}
#sleep 0.2
xdotool key $1