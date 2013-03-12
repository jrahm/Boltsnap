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

#define CMDVERSION ((1<<8)|26) 

int default_boltsnap_config( struct boltsnap_config* conf );


