/*
 * debug.c
 *
 *  Created on: Aug 20, 2012
 *      Author: jrahm
 */

#include "debug.h"

#include <string.h>
#include <fcntl.h>

#define SPACE ' '

int __debug_trace_on = 1;

FILE* __debug_log_file;
FILE* __debug_error_file;

void __debug_init(void) {
	__debug_log_file = stdin;
	__debug_error_file = stderr;
}

int debug_openlogfile( const char* filename ) {
	FILE* log = fopen( filename, "a+" );
	char mesg[1024];
	if( ! log ) {
		snprintf( mesg, 1024, "Cannot open log file '%s'", filename );
		perror(mesg);
		debug_setlogfile( stdout );
		return -1;
	}
	fcntl( fileno( log ), O_FSYNC );
	debug_setlogfile( log );
	return 0;
}

int debug_openerrfile( const char* filename ) {
	FILE* log = fopen( filename, "a+" );
	char mesg[1024];
	if( ! log ) {
		snprintf(mesg, 1024, "Cannot open error file '%s'", filename );
		perror(mesg);
		debug_seterrfile( stderr );
		return -1;
	}
	fcntl( fileno( log ), O_FSYNC );
	debug_seterrfile( log );
	return 0;
}

/*
 * The main stack where the frames are
 * pushed and popped on and off
 */
StackTrace stack = { NULL, 0, 10, NULL };

/*
 * Pushes a new frame `frame` onto the
 * stack pointed to by `sk`
 */
void push_frame( StackTrace* sk, StackFrame* frame ) {

	// if the number of elements is equal to the total size
	if( sk->n == sk->total_size || !sk->frames ) {
		StackFrame* fr = sk->frames;
		sk->frames = (StackFrame*) malloc( sizeof( StackFrame ) * (sk->total_size*=2) );

		char* old = sk->prefix;
		if(old) {
			free(old);
		}

		sk->prefix = malloc( sk->total_size + 1 );
		memset(sk->prefix, SPACE, sk->total_size + 1 );

		if( fr ) {
			size_t i = 0;
			for( ; i < sk->n; ++ i ) {
				sk->frames[i] = fr[i];
			}
			free(fr);
		}
	}

	sk->prefix[ sk->n ] = SPACE;
	sk->frames[ sk->n ++ ] = *frame;
	sk->prefix[ sk->n ] = 0;
}

/*
 * Pops a frame off the stack pointed to
 * by `sk`
 */
void pop_frame( StackTrace* sk ) {
	sk->prefix[ sk->n ] = SPACE;
	-- sk->n;
	sk->prefix[ sk->n ] = 0;
}

/*
 * Handles a signal by printing a stack
 * trace and exiting with the caught signal
 */
void handleSignal( int sig ) {
	__cperror( "Caught signal: %i\n", sig );
	__ifstacktrace( printStackTrace() );

	exit(sig);
}

/*
 * Sets the signals defined in `sigs` which
 * is a -1 terminated list of integers
 */
void setSignals(int sigs[]) {
	while( *sigs != -1 ){
		signal( *(sigs ++), handleSignal );
	}
}

/*
 * Sets the signals defined in COMMON_SIGNALS
 */
void setDefaultSignals() {
	int args[] = COMMON_SIGNALS;
	setSignals(args);
}

/*
 * Pushes a stack frame with the function name `func`,
 * file name `file` and the line `line` onto the stack trace
 */
__trace_depth_count_t __debug_itd( const char* func, const char* file, size_t line ) {
	StackFrame frame;

	frame.function = func;
	frame.file = file;
	frame.line = line;	

	push_frame( &stack, &frame );
	return stack.n;
}

/*
 * Pops a frame off the stack
 * and returns the size of the stack before
 * the pop
 */
__trace_depth_count_t __debug_dtd( ) {
	pop_frame( &stack );
	return stack.n + 1;
}

/*
 * returns the current traced depth of the
 * stacks
 */
__trace_depth_count_t __debug_td() {
	return stack.n;
}

/*
 * returns the current stack trace
 */
const StackTrace* getStackTrace() {
	return &stack;
}

/*
 * Prints the current stack trace to
 * stderr
 */
void printStackTrace() {
	fprintStackTrace( stderr, getStackTrace() );
}

/*
 * Prints the stack trace `stk' to the
 * file `file'
 */
void fprintStackTrace( FILE* file, const StackTrace* stk ) {

	fprintf(file, "Stack Trace:\n");
	StackFrame* frame = stk->frames + stk->n - 1;
	for( ; frame >= stk->frames ; -- frame ) {
		fprintf(file, "\tat: %s (%s:%u)\n", frame->function, frame->file, frame->line );
	}
}

/*
 * Returns a space for every frame
 * on the stack
 */
__debug_string_type __debug_prefix() {
	return stack.prefix == NULL ? "0.)" : stack.prefix;
}

