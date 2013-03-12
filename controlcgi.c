#include "common_cgi.h"
#include "control_command.h"

int main( int argc, char** argv ) {
	__debug_init();
	struct boltsnap_config conf;
	default_boltsnap_config( &conf );

	int fd = boltsnap_connect( &conf );
	unsigned int extension;

	struct boltsnap_command command;
	command.version = CMDVERSION;

	command.type = CONTROL;
	
	extension = argv[0] == NULL || argv[1] == NULL ?
					0 :  atoi(argv[1]);
	printf( "Using extension %d\n", extension );	
	if(extension > 2) {
		fprintf(stderr, "Invalid extension!\n");
		return 1;
	}

	command.extension = extension;

	command.packet_size = 0;
	command.packet = NULL;

	command.errno = 0;

	boltsnap_command_write( &command, fd, control_command_write );

	close(fd);

	return 0;
}
