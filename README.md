OFLmetrics
==========

# Introduction

This project provides code for running a simple network of sensors, based on MC1322x hardware (802.15.4 protocol)
I love simple things, so it's very low level ( Have a look at Contiki if you want an embedded OS )

Since I don't have much free time at the moment, I decided to publish it under open-source GPL licence.

Initial goals:


- Lightweight code (Not an OS: No tasks, processes, threads... just a main loop, and a couple of timers)
- Simple protocol (No TCP/IP things, just some raw bytes . Back to basics)
- Simple meshing (Routing over a a couple of nodes is enough for me) => Work in progress
- Some security (eg: XTEA software encryption ... )  => Work in progress
- Low power (Should operate properly on batteries)

Current status: 

- oflnode can read temperature,light sensors, and send messages through radio, and wake_up/sleep to save power
- ofldongle can listen to oflnode radio frames, and display/decode them on the serial port 
- oflbridge is a Qt daemon that turns on ofldongle monitor mode, decodes frames, and store them in MySQL server

Please come aboard and contribute ! 

# Documentation 

I'll try to write some detailed documentation soon. 

# OFLMetrics in action 

  * OFLnode 

![OFLnode](https://raw.githubusercontent.com/ofauchon/oflmetrics/master/docs/images/oflnode01.png)
![OFLnode](https://raw.githubusercontent.com/ofauchon/oflmetrics/master/docs/images/oflnode02.png)
![OFLnode](https://raw.githubusercontent.com/ofauchon/oflmetrics/master/docs/images/oflnode03.png)

  * OFLbridge

![OFLbridge hardware](https://raw.githubusercontent.com/ofauchon/oflmetrics/master/docs/images/oflbridge01.png)
![OFLbridge monitor mode](https://raw.githubusercontent.com/ofauchon/oflmetrics/master/docs/images/ofldongle_monitor_mode.png)



# Hardware 

I use econotags v1/v2, Redbee, M12. 
Please visit http://redwirellc.com to order 

# Components 


- oflnode   : The sensor firmware itself, it can reads or write from GPIOs, and send/receives frames from others.
- ofldongle : USB Stick to receive/send frames to/from OFLNodes (through serial port) 
- oflbridge : Communication daemon with ofldongle, to multiple backends (xpl, mysql)


# Notes

- No warranty, use at your own risk
- Sorry for the code quality/bugs you may find
- Please tell me if you do something useful with this code. 
- I don't sell hardware. go  http://redwirellc.com

