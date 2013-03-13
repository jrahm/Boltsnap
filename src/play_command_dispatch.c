#include "playthread.h"
#include "play_command.h"
#include "daemon.h"

#include <stdio.h>
#include "debug.h"

const struct cmdtbl_entry play_command_table_entry = 
(struct cmdtbl_entry){
	play_command_write,
	play_command_read,
	play_command_dispatch,
	play_command_destroy
};

/* This function will dispatch a play
	command by forking a new child to play
	the audio */
int play_command_dispatch( struct boltsnap_command* cmd ) {
	__cptrace("play_command_dispatch: START\n");

	if( cmd->type != PLAY ) {
		/* I think this has already been
			checked, but just to make sure */
		fprintf( stderr, "Command type not of type PLAY\n" );
		return -1;
	}
	
	/* Grab the packet */
	struct play_command* play_cmd = cmd->packet;
	
	struct playthread* thread = get_playthread_by_id(0);
	/* send a message to the
		play thread */
	playthread_request_play( thread, play_cmd->filename );

	__cptrace("play_command_dispatch: END\n");

	return 0;
}
