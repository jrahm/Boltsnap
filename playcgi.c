#include "daemon.h"
#include "boltsnap_cgi.h"
#include "play_command.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <string.h>
#include "debug.h"
#include "boltsnap.h"

#include <unistd.h>

FILE* implog;

#define CMDVERSION ((1<<8)|26) 

int main(int argc, char** argv) {
	__debug_init();
	__cpinfo("Starting executable playcgi\n");

	/* Check to make sure we have enough arguments */
	if( argc < 2 ) {
		__cpinfo( "No File Specified\n" );
		fprintf( stderr, "Need to provide the file to play\n" );
		return 1;
	}

	__cpinfo( "Opening fifo: " IMPULSE_FIFO );
	
	/* create the configuration */
	struct boltsnap_config conf;
	conf.port = 0;
	conf.type = AF_UNIX;
	conf.backlog = 5;
	conf.sockpath = IMPULSE_FIFO;

	/* read the filename from the arguments */
	const char* file_rel = argv[1];
	char *file_abs = NULL;
	
	/* get the real and full path of the file */
	if(! (file_abs = realpath( file_rel, file_abs )) ) {
		__cpinfo("Failed to get real path\n");
		fprintf(stderr, "Failed to get real path\n");
		return 2;
	}

	/* Connect to the boltsnap
		server using the configuration */
	int fd = boltsnap_connect( &conf );
	if( fd < 0 ) {
		__cperror( "Error on boltsnap_connect()\n" );
		return 1;
	}
	
	/* create the command to send
		across the pipe */
	struct boltsnap_command command;
	
	command.version = CMDVERSION;
	/* this is a binary for playing
		a song */
	command.type = PLAY;
	command.extension = 0;

	/* add the packet to the command */
	command.packet_size = sizeof( struct play_command );
	command.packet = play_command_new( file_abs );

	__cpinfo( "Writing %s to fifo.\n", file_abs );

	/* write the command to the file descriptor */
	boltsnap_command_write( &command, fd, play_command_write );	

	__cpinfo( "Closing fifo\n" );
	close( fd );

	// delete the file
	if( file_abs ) {
		free(file_abs);
	}

	return 0;
}
