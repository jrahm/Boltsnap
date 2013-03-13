#ifndef DAEMON_H_
#define DAEMON_H_

#define FOSNAP_FIFO "/tmp/boltsnap_local.sock"
#define FOSNAP_FIFO_PERMS 0777

#define IMPULSE_FIFO FOSNAP_FIFO
#define IMPULSE_FIFO_PERMS FOSNAP_FIFO_PERMS

#include "playthread.h"

struct playthread* get_playthread_by_id( int id );

int new_playthread();

#endif
