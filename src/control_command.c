
#include "control_command.h"

void* control_command_read( struct boltsnap_command* cmd, int fd ) {
	/* There is no packet with a control command (yet) */
	return NULL;
}

int control_command_destroy( void* play_cmd ) {
	/* There is nothing to do. */
	return 0;
}

int control_command_write( struct boltsnap_command* cmd, int fd ) {
	/* Nothing to do */
	return 0;
}
