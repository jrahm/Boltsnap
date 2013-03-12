#ifndef CONTROL_COMMAND_H_
#define CONTROL_COMMAND_H_

#include "boltsnap_command.h"

#define PAUSE_EXTENSION   0
#define RESUME_EXTENSION  1
#define STOP_EXTENSION    2

void* control_command_read( struct boltsnap_command* cmd, int fd );

int control_command_dispatch( struct boltsnap_command* command );

int control_command_destroy( void* play_cmd );

int control_command_write( struct boltsnap_command* cmd, int fd );

#endif
