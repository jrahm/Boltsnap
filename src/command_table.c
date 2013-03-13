#include "command_table.h"
#include <stdio.h>

int register_command( struct command_table* __cmdtbl, 
						enum command_type __type,
						const struct cmdtbl_entry* __entry ) {
	if( __type >= NOCMD + 1 ) {
		// the command we try to store does not exist
		fprintf(stderr, "Attempted to store entry outside of array bounds! (%s:%d)", __FILE__, __LINE__);
		return -1;
	}

	__cmdtbl->entries[__type] = *__entry;
	return 0;
}

const struct cmdtbl_entry* lookup_command( const struct command_table* __cmdtbl,
								enum command_type __type ) {
	if( __type >= NOCMD + 1 ) {
		// return null because this is out of bounds
		return NULL;
	}

	return &__cmdtbl->entries[__type];	
}
