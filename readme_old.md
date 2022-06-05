## FCBI:
	port FCBI filter( https://github.com/yoya/image.js/blob/master/fcbi.js ) to avisynth

### Version:
	0.0.0

### Usage:
```
	FCBI(clip, bool "ed", int "tm")
```
	clip: planar 8bit YUV(except YV411) only.
	ed: use edge detection or not (default = false).
	tm: threshold for edge detection (default = 30).
	
	When ed is set to false, tm will be ignored.

### Requirements:
	- Windows Vista sp2 or later
	- SSE2 capable CPU
	- Avisynth 2.60 / Avisynth+MT r2085 or later
	- Microsoft VisualC++ Redistributable Package 2015.

### License:
Copyright (c) 2016, OKA Motofumi <chikuzen.mo at gmail dot com>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

### Source code:
	https://github.com/chikuzen/FCBI/
