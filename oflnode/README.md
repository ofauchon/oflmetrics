OFLnode
=======

Current status: 

- oflnode can read temperature,light sensors, and send messages through radio, and wake_up/sleep to save power

# How to build

Ensure your Arm Toolchain is CodeSourcery arm-2008q3-66-arm-none-eabi.bin

$ wget 'wget https://sourcery.mentor.com/public/gnu_toolchain/arm-none-eabi/arm-2008q3-66-arm-none-eabi-i686-pc-linux-gnu.tar.bz2'
$ tar xvfz  arm-2008q3-66-arm-none-eabi-i686-pc-linux-gnu.tar.bz2

Build with 'make'

Flash with '../scripts/flash_m12 m12 oflnode_m12.bin'
Reset with the reset switch
Read console logs with 'screen /dev/ttyUSB1 115200'




