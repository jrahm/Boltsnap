#ifndef IMPULSE_CGI_H_
#define IMPULSE_CGI_H_

#include <stdio.h>

extern FILE* implog;

#define implogf( fmt, ... ) ( fprintf(implog, fmt, ##__VA_ARGS__) )

#endif
