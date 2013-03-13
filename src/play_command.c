#include "play_command.h"

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>

#include "debug.h"

#include "globals.h"

/*
 * The play command is just a string preceeded
 * by the length of said string.
 *
 * +-------------------+------------+-----+
 * | String length     | String ... | ... |
 * |   32-bit unsigned |        ... | ... |
 * +-------------------+------------+-----+
 *
 * The String should then be followed by a null
 * terminator.
 * The length should not include the null-terminator.
 */
void* play_command_read( struct boltsnap_command* cmd, int fd ) {
	__cptrace("start\n");

	/* dont want it to be zero in
		case it is not read */
	uint8_t handle = -1;

	/* Create the play command */
	struct play_command* ret = malloc( sizeof( struct play_command ) ) ;

	if( ! ret ) {
		fprintf( stderr, "Unable to allocate space for play command" );
		return NULL;
	}
	
	/* read in the length of the string */
	uint32_t len = 0;
	check_read( read(fd, &len, sizeof(uint32_t)),
		"Error on play_command_read\n", NULL );

	/* allocate space for the filename */
	char* filename = malloc( len + 1); /* Keep room for null termiantor */

	if( ! filename ) {
		fprintf( stderr, "Unable to allocate space for filename!\n" );
		free(ret);
		return NULL;
	}

	/* read the filename */
	check_read( read( fd, filename, len ),
		"Error on play_command_read\n", NULL );

	/* read the last byte; it should be 0 */
	check_read( read( fd, &handle, sizeof( uint8_t ) ),
		"Error on reading last byte in play_command_read\n", NULL);

	if( handle != 0 ) {
		fprintf( stderr, "Poorly formatted data! Missing NULL at end of string." );
		free(ret);
		free(filename);
		return NULL;
	}
	filename[len] = '\0';
	
	/* initialize ret */
	ret->str_length = len;
	ret->filename = filename;

	__cptrace("Created play_command with filename=%s\n", filename);
	__cptrace("end\n");

	return ret;
}

/* destroys a play command by
	freeing the filename to memory */
int play_command_destroy( void* play_cmd ) {
	__cptrace("play_command_destroy\n");

	struct play_command* cmd = play_cmd;
	free(cmd->filename);
	return 0;
}

int play_command_write( struct boltsnap_command* cmd2, int fd ) {
	struct play_command* cmd = cmd2->packet;
	size_t bytes_written = 0;

	bytes_written = write( fd, &cmd->str_length, sizeof( uint32_t ) );
	if( bytes_written == 0 ) {
		fprintf( stderr, "Error on writing play command!" );
		return 1;
	}

	/* do not forget the null terminator */
	bytes_written = write( fd, cmd->filename, cmd->str_length + 1 );
	if( bytes_written == 0 ) {
		fprintf( stderr, "Error on writing play command!\n");
		return 1;
	}

	return 0;
}

struct play_command* play_command_new( const char* str ) {
	size_t len = strlen( str );
	char* nstr = malloc( len + 1 );
	memcpy( nstr, str, len + 1 );
	struct play_command* ret = malloc( sizeof( struct play_command ) );

	ret->str_length = len;
	ret->filename = nstr;

	return ret;
}
