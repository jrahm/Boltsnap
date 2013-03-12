#include "daemon.h"

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "boltsnap.h"
#include "boltsnap_command.h"
#include "command_table.h"
#include "play_command.h"
#include "playthread.h"
#include "control_command.h"

#include "debug.h"

#define BOLTSNAPD_LOG "/var/log/boltsnap/boltsnapd.log"
#define BOLTSNAPD_ERR_LOG "/var/log/boltsnap/boltsnapd.err.log"

#define BUF_SIZE 128
#define MAX_PLAY_THREADS 8

struct command_table cmdtable;

int usedplaythreads = 1;
struct playthread playthreads[MAX_PLAY_THREADS];

struct playthread* get_playthread_by_id( int id ) {
	if( id > usedplaythreads ) {
		return NULL;
	}
	return &playthreads[id];
}

int new_playthread() {
	if( usedplaythreads == MAX_PLAY_THREADS ) {
		return -1;
	}
	playthread_init( &playthreads[usedplaythreads ++] );
	return usedplaythreads - 1;	
}

void main_callback( void* cmd, int exitcode ) {
	__cpinfo( "Command finished with exitcode: %d\n", exitcode );
	free(cmd);
}

void init() {
	__start_trace();

	memset(&cmdtable, 0, sizeof( cmdtable ) );

	struct cmdtbl_entry playentry = (struct cmdtbl_entry){
		play_command_write,
		play_command_read,
		play_command_dispatch,
		play_command_destroy
	};

	struct cmdtbl_entry controlentry = (struct cmdtbl_entry) {
		control_command_write,
		control_command_read,
		control_command_dispatch,
		control_command_destroy
	};

	register_command( &cmdtable, PLAY, &playentry );
	register_command( &cmdtable, CONTROL, &controlentry );

	playthread_init(&playthreads[0]);
	playthread_start(&playthreads[0]);

	__end_trace();
}

/*
 * This function is called every time
 * a new connection is made and therefore
 * everytime a new command is issued.
 *
 * This function is called asychronously
 */
void* main_connection_handler( struct boltsnap_connection conn ) {
	/* setup the context */
	struct command_context context;
	context.lookup_table = &cmdtable;
	
	/* the file descriptor to read from */
	int fd = conn.fd;
	
	/* read in the command */
	struct boltsnap_command* command = 
		read_command( fd, NULL, &context );

	__cptrace("Next command read and stored at: %p\n", command);
	
	/* dispatch the command if was successfully read */
	if( command ) {
		command_dispatch( command, &context, main_callback );
	} else {
		__cperror("There was a problem reading the next command!\n");
	}

	/* close the file descriptor */
	close(fd);

	return NULL;
}

int reader;

/* main function that starts the
	main boltsnapd server */
int main( int argc, char** argv ) {
	debug_openlogfile( BOLTSNAPD_LOG );
	debug_openerrfile( BOLTSNAPD_ERR_LOG );

	fprintf( __debug_error_file, "Testing Error!\n" );
	fprintf( __debug_log_file, "Testing log!\n" );
	
	printf( "Log file and Error file at: (%p,%p)\n", __debug_error_file, __debug_log_file );
	/* turn on trace to see debug output */
	__trace_on();
	__start_trace();
	
	/* setup whatever we need to */
	init();
	
	/* create a default configuration */
	struct boltsnap_config conf;
	conf.port = -1;
	conf.type = AF_UNIX;
	conf.backlog = 5;
	conf.sockpath = IMPULSE_FIFO;

	__cpinfo( "Opening socket on address " IMPULSE_FIFO "\n" );

	/* try to start the server */
	if( boltsnap_start_server( &conf, main_connection_handler) ) {
		return -1;
	}

	return 0;
}
