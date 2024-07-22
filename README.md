## RME Babyface pro input gain support

A continuation on [MrBollies](https://github.com/MrBollie) work on support for the RME Babyface Pro [(that is now in the mainline kernel)](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/sound/usb?h=v6.10-rc7&id=3e8f3bd047163d30fb1ad32ca7e4628921555c09).
One missing feature was to set input gain and thats what this patch enables. 

### Info

Line input gain has a 9db range and is set with 0.5 db incraments, 
and therefore set on the device with a value between 0 and 18.

The mic input gain however is a bit more odd.
It has a range from 0 to 65 db, and you would think it is set with a value from... well 0 to 65.
In decimal the sequencd makes no sence at all. In hex there is a pattern, and in binary it makes most sense.

There is 2 bits that are "rotating" between 00000000 00100000 and 01000000. 
And after each rotation the 5 least significant bits are incramented by 1. 

So first 
00000000 (0dB)
00100000 (1dB)
01000000 (2dB)

then 
00000001 (3dB)
00100001 (4dB)
01000001 (5dB)

00000010 (6dB)
00100010 (7dB)
01000010 (8dB)
etc.

it goes all the way up to 62, then it strays of this pattern.
If you look inside doc/usb gain messages.txt that anomoly makes sense if you look at the pattern in the hex values.

My only way to make sense of this is that it's made for "fine" and "coarse" control. Who knows. 

Some help/inspiration also taken from a [reverse engineering of the RME-Fireface-UC](https://github.com/agfline/RME-Fireface-UC-Drivers)
