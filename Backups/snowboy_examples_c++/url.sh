#!/bin/sh
# Activate chromium and press command line arg key
# Examples
# sh url.sh http://www.google.com

wid=$(xdotool search --onlyvisible --class chromium|head -1)
xdotool windowactivate ${wid}
xdotool key ctrl+l
xdotool type $1
xdotool key Return