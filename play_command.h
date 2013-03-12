#ifndef PLAY_COMMAND_H_
#define PLAY_COMMAND_H_

#include <inttypes.h>
#include <stdlib.h>

#include "boltsnap_command.h"
#include "command_table.h"

struct boltsnap_command;

extern int playing_child;

extern const struct cmdtbl_entry play_command_table_entry;

/*
 * Struct that represents a
 * play command in Impulse.
 */
typedef struct play_command {
	/* length of the string following */
	uint32_t str_length;
	
	/* the name of the file to play.
		this should arrive as null and
		this struct will be followed by the
		string. */
	char* filename;
} play_command_t;

/* returns a newly allocated play command
	from the file descriptor fd.
	the void* from this function can safely be
	casted to struct play_command**/
void* play_command_read( struct boltsnap_command* cmd, int fd );

/* runs a play command */
int play_command_dispatch( struct boltsnap_command* command );

/* destroys a play command */
int play_command_destroy( void* play_cmd );

int play_command_write( struct boltsnap_command* cmd, int fd );

struct play_command* play_command_new( const char* str );
#endif
