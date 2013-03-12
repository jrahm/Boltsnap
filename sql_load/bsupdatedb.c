#include "mediaload.h"

#include <string.h>
#include <sqlite3.h>

int main( int argc, char** argv ) {
	sqlite3* handle;

	if( !argv[0] || !argv[1] ) {
		fprintf( stderr, "Useage ./boltupdatedb <directory>\n" );
		return 1;
	}

	if( sqlite3_open( MEDIA_DB, &handle ) ) {
		perror("Error opening database "MEDIA_DB);
		return 1;
	}

	setup_database( handle );
	/* At this point it looks good */
	struct load_context context;
	context.directory = argv[1];;
	context.db = handle;
	
	printf("Loading music . . .\n");
	load_directory_root( &context );

	sqlite3_close( handle );

	return 0;
}
