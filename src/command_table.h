#ifndef COMMAND_TABLE_H_
#define COMMAND_TABLE_H_

#define NORDER_BITS 4
#define NORDER_BIT_MASK (~(0xFFFFFFFF<<NORDER_BITS))
#define NORDER_VALUE (1<<NORDER_BITS)

#include <inttypes.h>
#include <stdlib.h>
#include "boltsnap_command.h"

/* an entry into the command table */
struct cmdtbl_entry {
	/* writes to a file descriptor */
	int (*write)( struct boltsnap_command*, int );

	/* function that reads from a file descriptor */
	void *(*read) ( struct boltsnap_command*, int );
	
	/* function that dispatches the command */
	int (*dispatch) ( struct boltsnap_command* command );

	/* function to remove any allocated space */
	int (*destroy)( void* to_free );
};

//////////////////////
// AS OF NOW UNUSED //
//////////////////////
/* a bucket is the main node type used
	in a linked list implementation of the
	hashmap.

	entry: the entry associated with this hashtab
	next: the next entry in the linked list */
struct cmdtbl_bucket {
	/* entry associated with extension */
	struct cmdtbl_entry entry;

	/* the extension */ 
	uint32_t extension;

	/* the next bucket */
	struct cmdtbl_bucket* next;
};

struct cmdtbl_ext_hashtbl {
	/* the table entry for the case of no extension */
	struct cmdtbl_entry no_ext;

	/* if this command has an extension,
		here is the hashmap of those extensions */
	struct cmdtbl_bucket* buckets[NORDER_VALUE];
};
//////////////////////

struct command_table {
	/* the array where each bucket
		is stored */
	struct cmdtbl_entry entries[NOCMD+1];
};

/* adds a command to the command_table
	so it can later be looked up quickly when
	a new packet arrives */
int register_command( struct command_table* cmdtbl, enum command_type type,
						const struct cmdtbl_entry* entry );

/* returns the entry associated with
	the command type `type' */
const struct cmdtbl_entry* lookup_command( const struct command_table* cmdtbl,
								enum command_type type );

#endif
