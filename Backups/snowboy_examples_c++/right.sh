#!/bin/sh
# Activate firefox and press right arrow

wid=`xdotool search "Mozilla Firefox"`
xdotool windowactivate $wid
sleep 0.2
xdotool key Right