### Building GC in Linux running Raspberry Pi

This directory contains all the related bits to build GC in raspberry pis running Linux.

Since the [process](https://github.com/GoldenCheetah/GoldenCheetah/wiki/Building-Golden-Cheetah-on-a-Raspberry-Pi-4-on-a-fresh-Raspbian-Buster-install) has not been automated yet, we wrote a Makefile that captures all the necessary steps to build GC in a Raspberry Pi running raspbian. 
Hopefully this makes the process less error prone.

The targets (steps) to build GC are described in the Makefile. The first step does some housekeeping.  
You may want to reboot the machine after that and also after running the `gc/rules` target.

We currently work off of the 3.5 branch (GC repo). We will eventually catch up to the latest release.
So, the process may look like this: 

Open a terminal (as user pi) and run:

```sh
$ git clone https://github.com/GoldenCheetah/GoldenCheetah.git
$ cd GoldenCheetah/util/rpi
$ make -f Makefile.rpi gc/pre
# ...
# Wait for the processes to complete and *restart* the machine

$ make -f Makefile.rpi gc/deps gc/rules
# ...
# Wait for the processes to complete and *restart* the machine

$ make -f Makefile.rpi gc/sources gc/config gc/build gc/icon
# Wait for the processes to complete
```

If
the process completes successfully, you should have the GC binary in
`/home/pi/src/GoldenCheetah/src/GoldenCheetah`.

You can click on the Desktop icon to load GC or you can run it from the terminal. 
You may need to run it as sudo to have access to the usb2ant stick (if you use one).

