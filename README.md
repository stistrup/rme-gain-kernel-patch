## Kernel patch for RME Babyface pro to support input gain


A continuation on [MrBollies](https://github.com/MrBollie) work on support for the RME Babyface Pro [(that is now in the mainline kernel)](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/sound/usb?h=v6.10-rc7&id=3e8f3bd047163d30fb1ad32ca7e4628921555c09).
One missing feature was to set input gain and thats what this patch enables for Mic and Line in. 

Huge thanks to Andypoo which basically helped me through the whole process of making this and patching the kernel.

### Patching

Navigate to sound/usb in kernel source tree, and apply this patch (this patches mixer_quirks.c)\
Once patched, compile sound/usb module, install or load module and these controls should show up in alsa and be controllable:

Mic-AN1 Gain\
Mic-AN2 Gain\
Line-IN3 Gain\
Line-IN4 Gain

### Info

Line input gain has a 9db range and is set with 0.5 db incraments (0-18 on the usb bus)
Mic has 0-65 db range but has a quite [odd sequence of messages](https://github.com/stistrup/rme-gain-kernel-patch/blob/main/docs/usb%20gain%20messages.txt) which makes more sense in binary.

The 3 MSB are "rotating" between 00000000 00100000 and 01000000. 
And after each rotation the 5 LSB are incramented by 1. 

So first (starting at 0dB)\
00000000\
00100000\
01000000

then\
00000001\
00100001\
01000001

and\
00000010\
00100010\
01000010\
etc.

My only way to make sense of this is that it's split up like that for "fine" and "coarse" control. Where only changing the 5 LSB effectivly incraments 3 dB at time.
Maybe this is how the encoder on the physical device works, who knows. 

There is also an anomoly at the last 3dB. The pattern is followed all the way up to 62 db, then instead of incramenting the 5 LSB, it keeps incramenting the 3 MSB:\
(starting from 60 where the pattern starts)\
00010100
00110100
01010100
01110100
10010100
10110100

Some help/inspiration also taken from a [reverse engineering of the RME-Fireface-UC](https://github.com/agfline/RME-Fireface-UC-Drivers)
