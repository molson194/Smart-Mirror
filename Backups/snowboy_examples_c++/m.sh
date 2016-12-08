#!/bin/sh
# Activate firefox and paste the clipboard contents into the url bar.

wid=`xdotool search "Mozilla Firefox"`
xdotool windowactivate $wid
sleep 0.2
xdotool key m
