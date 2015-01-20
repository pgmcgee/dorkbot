#!/bin/sh

gcc -framework CoreAudio -framework AudioToolbox -framework CoreFoundation -framework ApplicationServices -g -o cmd cmd.c
cp cmd public/cmd
cp missile.py public/missile.py
