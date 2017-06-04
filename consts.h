#pragma once

#define BUFSIZE 1024

#define HEIGHT 1

#define __PROFILE 0

#define MMAAXX(a,b)	  \
	({ __typeof__ (a) _a = (a); \
		__typeof__ (b) _b = (b); \
		_a > _b ? _a : _b; })
