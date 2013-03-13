#ifndef IMPULSE_COMMAND_H_
#define IMPULSE_COMMAND_H_

#include <inttypes.h>
#include <stdlib.h>

#include <pthread.h>
#define STUB_SIZE ( sizeof( struct boltsnap_command ) - (sizeof( void * )<<1) );
#define check_malformed( __cond, __s, __wtd ) \
	if( ! (__cond) ) { \
		__cperror( __s ); \
		__wtd; \
	}

/* callback for dispatched functions */
typedef void (*callback_t)( void* cmd, int status );

/* this will be defined later */
struct command_table;
struct cmdtbl_entry;

/* context to be used for invoking commands */
struct command_context {
	struct command_table* lookup_table;
};

/* possible command types */
typedef enum command_type

{   PLAY, CONTROL, STOP,
    NEXT, PREV, SHUFFLE,
    REPEAT, REPEAT_ALL,
    CUSTOM, NOCMD	}

command_type_t;

/*
 * struct which contains information
 * to communicate with the running daemon
 */
typedef struct boltsnap_command {
    /* version of this protocol */
    uint32_t version;

    /* the type of command. Determines which of
        the following to use */
    enum command_type type;
	
	/* an extension of the type
		for use with custom commands */
	uint32_t extension;

    /* size of the following packet
        data */
	uint32_t packet_size;

	/* a handle to the entry in the table
		if it exists */
	const struct cmdtbl_entry*
		table_entry;

	/* the packet sent. This
		should arrive as null
		and be set to the struct
		that follows after */
	void* packet;

	/* defines whether or not
		there has been an error */
	int errno;
} boltsnap_command_t;

/* reads a command from the file descriptor fd
	if cmd is not null, then the new command is read
	into cmd, if it is null, then cmd will be malloc'd
	and returned */
struct boltsnap_command* read_command( int fd, struct boltsnap_command* cmd,
										struct command_context* context );

/* destroys and boltsnap_command by freeing the memory
	of the packet back to the heap */
int boltsnap_command_destroy( struct boltsnap_command* cmd,
								struct command_context* context );

/* this function will run the command in a separate thread */
void command_dispatch( struct boltsnap_command* cmd,
						struct command_context* context,
						callback_t callback );

/* writes a command to a file descriptor in the 
	standard form defined in the manual and in
	boltsnap_command.c */
int boltsnap_command_write( struct boltsnap_command* cmd, int fd,
							int (*write_func)( struct boltsnap_command*, int ) );

/* hashes a command to test against when being read
	through */
uint64_t boltsnap_command_hash( struct boltsnap_command* command );


char* boltsnap_command_print( char* buf, const struct boltsnap_command* cmd );

#endif
