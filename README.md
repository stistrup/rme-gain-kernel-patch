## Kernel patch for RME Babyface pro to support input gain


A continuation on [MrBollies](https://github.com/MrBollie) work on support for the RME Babyface Pro [(that is now in the mainline kernel)](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/sound/usb?h=v6.10-rc7&id=3e8f3bd047163d30fb1ad32ca7e4628921555c09).
One missing feature was to set input gain and thats what this patch enables for Mic and Line in. 

Huge thanks to Andypoo which basically helped me through the whole process of making this and patching the kernel.

---

### Patching

Navigate to sound/usb in kernel source tree, and apply this patch (this patches mixer_quirks.c)\
Once patched, compile sound/usb module, install or load module and these controls should show up in alsa and be controllable:

Mic-AN1 Gain\
Mic-AN2 Gain\
Line-IN3 Gain\
Line-IN4 Gain

---

### Findings (if you're interested)

I had TotalMix in a VM and sniffed the USB with wireshark to figure out how the messages is sent.

The values are sent as 16 bit messages as per the USB standard, but only 8 bit are relevant. 
It sends the messages to the same addresses in CC mode (if the feature is supported).

Line input gain has a 9db range and is set with 0.5 db incraments, so the range is doubled to 0-18 to accound for the half steps.

Mic has 0-65 db range but has a quite [odd sequence of messages](https://github.com/stistrup/rme-gain-kernel-patch/blob/main/docs/usb%20gain%20messages.txt) which makes more sense in binary.

The 3 highest-order bits are "rotating" between 000xxxxx, 001xxxxx, and 010xxxxx, then back to 000xxxxx. 
After each rotation, the 5 lowest-order bits are incremented by 1, xxx00001, xxx00010, xxx00011 etc.

So:\
0dB = 00000000\
1dB = 00100000\
2dB = 01000000

then\
3dB = 00000001\
4dB = 00100001\
5dB = 01000001

and\
6dB = 00000010\
7dB = 00100010\
8dB = 01000010\
etc.

My only way to make sense of this is that it's split up like that for "fine" control going by the sequence explained above, and "coarse" control only changing the 5 lowest-order bits, effectivly incraments 3 dB at time. But i can't find this coarse thing being used anywhere but it's the only way i can make sense of this sequence. 

There is also an anomoly at the last 3dB. The pattern is followed all the way up to 62 db, then 
instead of incrementing the 5 lowest-order bits, it keeps incrementing the 3 highest-order bits. 
So starting from 60 dB (where the normal pattern begins):

00010100\
00110100\
01010100\
01110100\
10010100\
10110100

I guess it doesn't really matter what RME had in mind, as long as it works.

Some help/inspiration also taken from a [reverse engineering of the RME-Fireface-UC](https://github.com/agfline/RME-Fireface-UC-Drivers)
