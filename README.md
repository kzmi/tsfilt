tsfilt
======

MPEG TS packet filter

Usage
-----

    tsfilt [[input] output]

  `input` : specifies a source TS file. if omitted, TS is read from stdin.

  `output` : specifies a file to output filtered TS. if omitted, filtered TS is written to stdout.


Description
-----------

`tsfilt` reads MPEG-TS and remove some elementary streams by dropping packets.

Currently, elementary streams other than the following streams are dropped.

 * Primary ISO/IEC 13818-2 (MPEG2 Video) stream
 * Primary ISO/IEC 13818-7 (MPEG2 AAC) stream

All supplementary streams are dropped.

`tsfilt` doesn't modify PAT and PMT. It only drops packets.


Examples
--------

To convert a file:

    tsfilt xxxxxxxx.ts yyyyyyyy.ts

To modify TS before convert it:

    tsfilt xxxxxxxx.ts | mencoder -ovc x264 -oac mp3lame -of avi -o xxxxxxxx.avi - 


