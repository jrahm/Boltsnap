#include "common_cgi.h"

int default_boltsnap_config( struct boltsnap_config* conf ) {
	conf->port = 0;
	conf->type = AF_UNIX;
	conf->backlog = 5;
	conf->sockpath = IMPULSE_FIFO;

	return 0;
}
