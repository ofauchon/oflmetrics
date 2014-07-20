#!/bin/sh
LD_LIBRARY_PATH=../xPLLib \
../xplbridge -l /tmp/xplbridge.log -p /dev/ttyUSB1 -s 115200 -t 5555 -H localhost -D oflmetrics -U oflmetrics -P Uraedoh3
