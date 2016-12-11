#!/bin/bash
python -m SimpleHTTPServer 8000 &
firefox localhost:8000/welcome.html