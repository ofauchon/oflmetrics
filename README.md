OFLmetrics
==========

# Introduction

This project provides code for running a simple network of sensors, based on MC1322x hardware (802.15.4 protocol)
I love simple things, and I decide to write my own protocol, two years ago. 

Since I don't have much free time at the moment, I decided to publish it under open-source GPL licence.

Initial goals:


- Lightweight (No tasks, processes, threads... just a main loop)
- Simple protocol (No TCP/IP things, just some raw bytes . Back to basics)
- Simple meshing (At the moment, routing over a a couple of nodes is enough for me)
- Basic security (eg: encryption ... ) 
- Low power (Should operate properly on batteries)

Current status: 

- OFLNode can read temperature sensors, and send messages, and wake_up/sleep
- OFLDongle can listen to OFLNode frames, and display/decode them on the serial port. 



Please come and contribute ! 

# Documentation 

I'll try to write some detailed documentation soon. 


# Hardware 

I use econotags v1/v2, Redbee, M12. 
Please visit http://redwirellc.com to order 

# Components 


- OFLNode : The sensor firmware itself, it can reads or write from GPIOs, and send/receives frames from others.
- OFLDongle : USB Stick to receive/send frames to/from OFLNodes (through serial port) 


# Notes

- No warranty, use at your own risk
- Sorry for the code quality/bugs you may find
- Please tell me if you do something useful with this code. 
- I don't sell hardware. go  http://redwirellc.com

