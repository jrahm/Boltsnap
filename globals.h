#ifndef GLOBALS_H_
#define GLOBALS_H_

#include <stdio.h>
#include "debug.h"

#define likely( a ) __builtin_expect((a), 1)
#define unlikely( a ) __builtin_expect((a), 0)

#define check_read( a, s, r ) if( unlikely((a) == 0 ) ) { __cperror(s) ; return r; }
#define check_write( a, s, r ) check_read( a, s, r )
#define check_read2( a, s, c ) if( unlikely((a) == 0 ) ) { __cperror(s) ; c; }

#endif
