OFLmetrics
==========

# Introduction

This project provides code for running a simple network of sensors, based on MC1322x hardware (802.15.4 protocol)
It's very low level: just a simple C code main() function and timers... Keep It Simple, Stupid !

It's open-source licensed, so please contribute !

Initial goals:


- Lightweight code (No OS, no tasks, no processes, no threads... just a main loop, and a couple of timers)
- Simple protocol (No TCP/IP , just some raw bytes over the air. Back to basics)
- Simple meshing (Lightweight routing protocol over a a couple of nodes to extend range) => Work in progress
- Some security (simple software encryption )
- Low power (Should operate properly on batteries)

Current status: 

- oflnode can wake up, read sensors's values, transmit them in a 802.15.4 packet and go sleep.
- ofldongle can listen to radio messages, and relay them to the computer through the serial port
- oflbridge is a Qt daemon that connects ofldongle 's serial port, decode sensor's values and send them to InfluxDB's database.


# Documentation 

I'll try to write some detailed documentation soon. 

# OFLMetrics suite in action 

  * OFLnode firmware running on Redbee Boards

![OFLnode](https://raw.githubusercontent.com/ofauchon/oflmetrics/master/docs/images/oflnode01.png)
![OFLnode](https://raw.githubusercontent.com/ofauchon/oflmetrics/master/docs/images/oflnode02.png)
![OFLnode](https://raw.githubusercontent.com/ofauchon/oflmetrics/master/docs/images/oflnode03.png)

  * OFLDongle firmware running on Econotag

![OFLbridge hardware](https://raw.githubusercontent.com/ofauchon/oflmetrics/master/docs/images/oflbridge01.png)

  * OFLBridge service receiving datas 

![OFLbridge monitor mode](https://raw.githubusercontent.com/ofauchon/oflmetrics/master/docs/images/ofldongle_monitor_mode.png)


  * InfluxDB time-series database & Grafana visualisation tool

![OFLbridge to InfluxDB](https://raw.githubusercontent.com/ofauchon/oflmetrics/master/docs/images/grafana01.png)


# Hardware 

- I use econotags v1/v2, Redbee, M12. 
- MC1322x chipset familly is deprecated. Please tell me if you use this code on other chips.

# Source code layout:

- /oflnode   : The sensor firmware itself, it can reads or write from GPIOs, and send/receives frames from others.
- /ofldongle : USB Stick to receive/send frames to/from OFLNodes (through serial port) 
- /oflbridge : Communication daemon with ofldongle, to multiple backends (xpl, mysql, influxDB - Work in progress)


# Notes

- No warranty, use at your own risk
- Please tell me if you do something useful with this code. 
- I don't sell hardware, sorry ! 
- Contributions, ideas, contributions,  welcome


# Q&A 

- Q: I get 'make: arm-none-eabi-gcc: Command not found' when 'make'
- A: Unpack arm compiler in arm-2008q3, then add arm-2008q3/bin to your path


