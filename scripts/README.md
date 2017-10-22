scripts
=======


  * ARM Toolchain 
```
wget 'https://sourcery.mentor.com/public/gnu_toolchain/arm-none-eabi/arm-2008q3-66-arm-none-eabi.bin'
arm-2008q3-66-arm-none-eabi.bin -i console
<follow instructions>
export PATH=~/CodeSourcery/Sourcery_G++_Lite/bin:$PATH

```


  * flasher

Tools require some dependancies: 

- Perl term/readkey and device/serialport libraries

pacman -S perl-device-serialport  perl-term-readkey

- Build some tools 
cd ../libmc1322x/tools/ftditools/ && make
cd ../libmc1322x/test/; make 


