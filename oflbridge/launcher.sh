#!/bin/bash

USER=oflmetrics
PASS=changeme

echo "Starting oflbridge"
./oflbridge -s /dev/ttyUSB1:115200 -i 'http://localhost:8086/write?db=oflmetrics' -d -l /var/log/oflmetrics/oflbridge.log
