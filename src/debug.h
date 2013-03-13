/*
 * debug.h
 *
 * NOTE: This file is NOT currently tailored to handle multiple threads!
 *
 *  Created on: Aug 2, 2012
 *      Author: Joshua Rahm
 *
 *  Header file with debugging utility functions in it for
 *  different levels. There are 6 logging levels:
 *
 *  VERBOSE: Use VERBOSE only for cases of extreme need to see exactly what is going in
 *  TRACE: Use TRACE to be able to see exactly where the program is going
 *  DEBUG: Use DEBUG to print statements that should be used to debug algorithms in the program
 *  INFO: Use INFO to notify the user of an important event, this is the level which is enabled by default
 *  WARN: Use WARN when a non-fatal, but troublesome problem has occurred
 *  ERROR: Use ERROR when a possible, non-recoverable error as occurred
 *
 *  Additionally, there is STACK_TRACE which is enabled if TRACE is enabled, but defined alone,
 *  STACK_TRACE will silently push a frame onto the trace stack so if the program crashes, a stack
 *  frame will still appear.
 *
 *  If a lower logging level is defined, all the levels above it are also defined
 *
 *  Macros:
 *
 *  DEBUG_NAMESPACE:	debug:: if C++ ; <nothing> if C
 *  					Use preceding each reference to a function in the header file to ensure compatible code
 *  					eg. DEBUG_NAMESPACE printStackTrace()
 *
 *  as_c_str(a):		a.c_str() if C++ ; a if C
 *  					No reason to use this outside of this file
 *
 *  __ifverbose( a ):	a if VERBOSE is defined, <nothing> otherwise
 *  __iftrace( a ):		a if TRACE is defined, <nothing> otherwise
 *  __ifdebug( a ):		a if DEBUG is defined, <nothing> otherwise
 *  __ifinfo( a ):		a if INFO is defined, <nothing> otherwise
 *  __ifwarn( a ):		a if WARN is defined, <nothing> otherwise
 *  __iferror( a ):		a if ERROR is defined, <nothing> otherwise
 *  __ifstacktrace( a ):a if STACK_TRACE is defined, <nothing> otherwise
 *
 *  __cpverbose( fmt, ... ):	acts exactly like printf if VERBOSE is defined, otherwise nothing
 *  __cptrace( fmt, ... ):	acts exactly like printf if TRACE is defined, otherwise nothing
 *  __cpdebug( fmt, ... ):	acts exactly like printf if DEBUG is defined, otherwise nothing
 *  __cpinfo( fmt, ... ):	acts exactly like printf if INFO is defined, otherwise nothing
 *  __cpwarn( fmt, ... ):	acts exactly like printf if WARN is defined, otherwise nothing
 *  __cperror( fmt, ... ):	acts exactly like fprintf(stderr, fmt, ... ) if ERROR is defined, otherwise nothing
 *
 *  __pverbose( a ):	std::cout << a << std::endl if VERBOSE is defined, otherwise nothing (C++ only)
 *  __ptrace( a ):	std::cout << a << std::endl if TRACE is defined, otherwise nothing (C++ only)
 *  __pdebug( a ):	std::cout << a << std::endl if DEBUG is defined, otherwise nothing (C++ only)
 *  __pinfo( a ):	std::cout << a << std::endl if INFO is defined, otherwise nothing (C++ only)
 *  __pwarn( a ):	std::cout << a << std::endl if WARN is defined, otherwise nothing (C++ only)
 *  __perror( a ):	std::cerr << a << std::endl if ERROR is defined, otherwise nothing (C++ only)
 *
 *  __start_trace():	Use at the beginning of every function which will be traced on the stack
 *  __end_trace():		Use at the end of every function which will be traced on the stack
 *
 *  __trace_return:		Short for __end_trace() ; return
 *
 *  __passert( a, c ):	If the condition `a' fails, print `c' using __perror and exit with code 1
 *  __cpassert( a, c ): If the condition `a' fails, print `c' using __cperror and exit with code 1
 *
 *  COMMON_SIGNALS:		Common signals used to signal a termination
 *
 *  Functions:
 *
 *  Only really need to know:
 *
 *  getStackTrace():					returns the current stack trace
 *  printStackTrace( [os], [stack] ):	prints a stack trace to an output stream
 */

#ifndef DEBUG_H_
#define DEBUG_H_

/*
 * Automatically include info statements
 * unless the macro NOINFO is defined
 */
#ifndef NOINFO
#define INFO
#endif

#ifdef VERBOSE

#ifndef TRACE
#define TRACE
#endif

#endif

#ifdef TRACE

#ifndef DEBUG
#define DEBUG
#endif

#endif

#ifdef DEBUG

#ifndef INFO
#define INFO
#endif

#endif

#ifdef INFO

#ifndef WARN
#define WARN
#endif

#endif

#ifdef WARN

#ifndef ERROR
#define ERROR
#endif

#endif

// Include correct C++ Headers
#ifdef __cplusplus

#include <cstdio>
#include <cstdlib>
#include <string>

#include <csignal>
#include <vector>
#include <iostream>

#include <cassert>

#define as_c_str(a) a .c_str()

#define DEBUG_NAMESPACE debug::
// Include correct C headers
#else

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <assert.h>

extern int __debug_trace_on;

extern FILE* __debug_log_file;
extern FILE* __debug_error_file;

#define DEBUG_LOG_FILE (__debug_log_file == NULL ? stdout : __debug_log_file)
#define DEBUG_ERR_FILE (__debug_error_file == NULL ? stderr : __debug_error_file)

void __debug_init(void);;

#define __trace_on() __debug_trace_on = 1
#define __trace_off() __debug_trace_on = 0

#ifndef __cplusplus
#define trace() __debug_trace_on
#endif

#define DEBUG_NAMESPACE
#define as_c_str(a) a
#endif

// Handle trace define
#ifdef TRACE

// define stack trace
#define STACK_TRACE
#define __iftrace(a) a
#define __ifntrace(a)
#define __trace(a) if( DEBUG_NAMESPACE trace() ) { fprintf(DEBUG_LOG_FILE, "    %s[TRACE]- %s:%d %s$ ", \
						as_c_str( DEBUG_NAMESPACE __debug_prefix() ), __FILE__, __LINE__, __FUNCTION__); a; \
						fflush( DEBUG_LOG_FILE );}
#else
#define __iftrace(a)
#define __ifntrace(a) a
#define __trace(a)
#endif

// Handle debug define
#ifdef DEBUG
#define __ifdebug(a) a
#define __debug(a) {fprintf(DEBUG_LOG_FILE, "    %s[DEBUG]- %s:%d %s$ ",  as_c_str( DEBUG_NAMESPACE __debug_prefix() ), __FILE__, __LINE__, __FUNCTION__); a; \
						fflush( DEBUG_LOG_FILE );}
#define __ifndebug(a)
#else
#define __ifdebug(a)
#define __debug(a)
#define __ifndebug(a) a
#endif

// Handle info define
#ifdef INFO
#define __ifinfo(a) a
#define __info(a)  {fprintf(DEBUG_LOG_FILE, "    %s[INFO] - %s:%d %s$ ",  as_c_str( DEBUG_NAMESPACE __debug_prefix() ),\
						__FILE__, __LINE__, __FUNCTION__); a;fflush( DEBUG_LOG_FILE );}
#define __ifninfo(a)
#else
#define __ifinfo(a)
#define __info(a)
#define __ifninfo(a) a
#endif

// Handle error define
#ifdef ERROR
#define __iferror(a) a
#define __error(a) {fprintf(DEBUG_ERR_FILE, "    %s[ERROR]- %s:%d %s$ ",  as_c_str( DEBUG_NAMESPACE __debug_prefix() ),\
						__FILE__, __LINE__, __FUNCTION__); a; fflush(DEBUG_ERR_FILE);};
#define __ifnerror(a)
#else
#define __iferror(a)
#define __error(a)
#define __ifnerror(a) a
#endif

// Handle war define
#ifdef WARN
#define __ifwarn(a) a
#define __warn(a) {fprintf(DEBUG_LOG_FILE, "    %s[WARN] - %s:%d %s$ ",  as_c_str( DEBUG_NAMESPACE __debug_prefix() ), \
					__FILE__, __LINE__, __FUNCTION__); a; fflush(DEBUG_ERR_FILE);}
#define __ifnwarn(a)
#else
#define __ifwarn(a)
#define __warn(a)
#define __ifnwarn(a) a
#endif

#ifdef VERBOSE
#define __ifverbose(a) a
#define __verbose(a) {printf(DEBUG_LOG_FILE, "    %s[VERBOSE] - %s:%d %s$ ",  as_c_str( DEBUG_NAMESPACE __debug_prefix() )i\
						, __FILE__, __LINE__, __FUNCTION__); a;fflush( DEBUG_LOG_FILE );}
#define __ifnverbose(a)
#else
#define __ifverbose(a)
#define __verbose(a)
#define __ifnverbose(a) a
#endif

// define print functions for C, not C++
#define __cpdebug( fmt, ... )	__debug(fprintf(DEBUG_LOG_FILE, " "); fprintf(DEBUG_LOG_FILE, fmt, ##__VA_ARGS__))
#define __cpinfo( fmt, ... )		__info (fprintf(DEBUG_LOG_FILE, " "); fprintf(DEBUG_LOG_FILE, fmt, ##__VA_ARGS__))
#define __cperror( fmt, ... )	__error(fprintf(DEBUG_ERR_FILE, " "); fprintf(DEBUG_ERR_FILE, fmt, ##__VA_ARGS__))
#define __cptrace( fmt, ... )	__trace(fprintf(DEBUG_LOG_FILE, " "); fprintf(DEBUG_LOG_FILE, fmt, ##__VA_ARGS__))
#define __cpwarn( fmt, ... )		__warn (fprintf(DEBUG_LOG_FILE, " "); fprintf(DEBUG_LOG_FILE, fmt, ##__VA_ARGS__))
#define __cpverbose( fmt, ... )		__verbose (fprintf(DEBUG_LOG_FILE, " "); fprintf(DEBUG_LOG_FILE, fmt, ##__VA_ARGS__))

// define print functions for C++
#ifdef __cplusplus
// Define the print macros using C++ iostreams
// note syntax like __pdebug( "Hello" << ", " << "World!" ) is perfectly valid!
#define __pdebug( chrs )	__debug( std::cout << chrs << std::endl )
#define __pinfo( chrs )		__info ( std::cout << chrs << std::endl )
#define __perror( chrs )	__error( std::cerr << chrs << std::endl )
#define __ptrace( chrs )	__trace( std::cout << chrs << std::endl )
#define __pwarn( chrs )		__warn ( std::cout << chrs << std::endl )
#define __pverbose( chrs )		__verbose ( std::cout chrs << std::endl )

#endif // #ifdef __cplusplus

#ifdef STACK_TRACE
#define __ifstacktrace(a) a
#define __start_trace()		if( DEBUG_NAMESPACE trace() ) { __iftrace( \
								fprintf(DEBUG_LOG_FILE, "\n") ; fprintf(DEBUG_LOG_FILE, "%i.) %s<%s>\n", DEBUG_NAMESPACE __debug_itd(__PRETTY_FUNCTION__, __FILE__, __LINE__), as_c_str( DEBUG_NAMESPACE __debug_prefix() ) , __PRETTY_FUNCTION__)\
							); __ifntrace ( \
									 DEBUG_NAMESPACE __debug_itd(__PRETTY_FUNCTION__, __FILE__, __LINE__)\
							) }

#define __end_trace()		if( DEBUG_NAMESPACE trace() ) { __iftrace( fprintf(DEBUG_LOG_FILE, "%i.)%s</%s>\n\n", DEBUG_NAMESPACE __debug_td(), as_c_str( DEBUG_NAMESPACE __debug_prefix() ), __PRETTY_FUNCTION__)); DEBUG_NAMESPACE __debug_dtd(); }
#else
#define __ifstacktrace(a)
#define __start_trace()
#define __end_trace()
#endif

#define __trace_return __end_trace() ; return

// Calls assert, 
#define __passert(a,c) if(!(a)){ __perror(c); DEBUG_NAMESPACE handleSignal(1); }
#define __cpassert(a,c) if(!(a)){ __cperror(c); DEBUG_NAMESPACE handleSignal(1); }

// The most commonly used signals
#define COMMON_SIGNALS {SIGSEGV, SIGBUS, SIGSYS, SIGQUIT, SIGILL, SIGABRT, SIGTERM, -1}

#ifdef __cplusplus
namespace debug {
#endif

typedef int __trace_depth_count_t;

#ifdef __cplusplus
typedef std::string __debug_string_type;
#else
typedef const char* __debug_string_type;
#endif

// struct that defines a stack frame
struct __stack_frame {
	// The function name of this stack frame
	const char* function;

	// the file this stack_frame is in
	const char* file;

	// the line of the __start_trace() statement
	__trace_depth_count_t line;

// Constructors
#ifdef __cplusplus
	// construct a new stack_frame with the function name, filename, and line number
	__stack_frame( const char*, const char*, size_t );

	// copy constructor
	__stack_frame( const __stack_frame& sf );

	// assignment operator
	__stack_frame& operator=( const __stack_frame& sf );
#endif
};

typedef struct __stack_frame StackFrame;

#ifdef __cplusplus
typedef std::vector<StackFrame> StackTrace;
#else
// type used to store the stack frames for archaic C
struct __debug_stack_trace_stack {
	StackFrame* frames;
	size_t n;
	size_t total_size;
	char *prefix;
};
// typedef it to StackTrace
typedef struct __debug_stack_trace_stack StackTrace;

#endif

// push a frame onto the stack
void push_frame( StackTrace* stack, StackFrame* sf );

// pop a frame from the stack
void pop_frame( StackTrace* stack );

/*
 * Sets this program to handle the signals
 * passed with handleSignal(int)
 */
void setSignals( int signals[] );

/*
 * Calls `setSignals` defined above with
 * COMMON_SIGNALS
 */
#ifdef __cplusplus
void setSignals();
inline void setDefaultSignals() { setSignals(); }
#endif

/*
 * Prints the current stack trace
 * and exits the program with the integer
 * passed
 */
void handleSignal(int);

/*
 * Pushes a new stack frame onto the
 * stack.
 */
 __trace_depth_count_t __debug_itd( const char* func, const char* file, size_t line );

/*
 * Pops a stack frame from the stack
 */
 __trace_depth_count_t __debug_dtd();

/*
 * returns the number of stack frames on
 * the stack.
 */
 __trace_depth_count_t __debug_td();

/*
 * Returns a string with a space for every
 * frame on the stack
 */
__debug_string_type __debug_prefix();

/*
 * Returns the global stack trace
 */
const StackTrace* getStackTrace();

void trace_off();

void trace_on();

int debug_openlogfile( const char* filename );

int debug_openerrfile( const char* filename );

#define debug_setlogfile( logfile ) __debug_log_file = (logfile);
#define debug_seterrfile( errfile ) __debug_error_file = (errfile);

#ifdef __cplusplus
inline int trace() { return __debug_trace_on; }
#endif

void cloneStackTrace( StackTrace* stk_trace );

#ifdef __cplusplus
/*
 * Prints the stack trace to the output stream `os`
 */
std::ostream& printStackTrace(std::ostream& os=std::cerr, const StackTrace* stk=getStackTrace() );

/*
 * Prints the stack trace to the output stream `os`
 */
inline std::ostream& operator<<(std::ostream& os, const StackTrace& stk ) { return printStackTrace( os, &stk ); }
#else

/*
 * If the language is C, we need to define print stack trace
 * that accepts no arguments
 */
void printStackTrace();
#endif

void fprintStackTrace( FILE* file, const StackTrace* stk );

#ifdef __cplusplus
} // End namespace debug
#endif

#endif /* DEBUG_H_ */
