#!/bin/bash

USER=oflmetrics
PASS=changeme

while true; do 
  echo "Starting oflbridge"
  LD_LIBRARY_PATH=./xPLLib ./oflbridge -s /dev/ttyUSB1:115200 -i 'http://localhost:8086/write?db=oflmetrics' 2>&1 | ts
  echo "Sleep 60" ;sleep 60
done
