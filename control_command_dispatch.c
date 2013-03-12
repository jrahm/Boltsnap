#include "control_command.h"
#include "playthread.h"

#include "daemon.h"
#include "debug.h"

int control_command_dispatch( struct boltsnap_command* command ) {
	struct playthread *thread = get_playthread_by_id( 0 );

	__cptrace( "command->extension=%d\n", command->extension );

	switch( command->extension ) {
	case PAUSE_EXTENSION:
		playthread_pause_playback( thread );
		break;
	case RESUME_EXTENSION:
		playthread_resume_playback( thread );
		break;
	case STOP_EXTENSION:
		playthread_stop_playback( thread );
		break;
	default:
		__cperror("The extension %d is not mapped to any "
						"operation of control command",
							command->extension);
	}
	return 0;
}
