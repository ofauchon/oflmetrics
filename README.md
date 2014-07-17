OFLmetrics
==========

# Introduction

This project provides code for running a simple network of sensors, based on MC1322x hardware (802.15.4 protocol)
I love simple things, and I decide to write my own protocol, two years ago. Goals were: 


- KISS compatibility
- Lightweight (No tasks, processes, threads... just a main loop)
- Simple protocol (No TCP/IP things, just some raw bytes . Back to basics)
- Simple meshing (At the moment, routing over a a couple of nodes is enough for me)
- Basic security (eg: encryption ... ) 
- Low power (Should operate properly on batteries)

Since I don't have much time to spend on this project, I decided to publish it under GPL licence. 

Any contributers welcome ! 

# Documentation 

I'll try to write some more documentation soon. 


# Components 


- OFLNode : The sensor firmware itself, it can reads or write from GPIOs, and send/receives frames from others.
- OFLDongle : USB Stick to receive/send frames to/from OFLNodes (through serial port) 





