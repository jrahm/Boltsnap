#include "boltsnap_command.h"
#include "command_table.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <assert.h>
#include <pthread.h>
#include "debug.h"
#include "globals.h"

#define DELETE_IF_NEED if( cmd2 == NULL ) { free(cmd); } return NULL;


#define MAX_PACKET_SIZE 256

/*
 * The layout of the command as of version 0.1.27
 *
 * 
 * +------------------+------------------+-------------------+------------------+
 * | Protocol Version | Command-Type     | Type Extension    | Packet Size      |
 * |  32-bit unsigned |  8-bit  unsigned |   32-bit signed   |  32-bit unsigned |
 * +------------------+------------------+-------------------+------------------+
 * 
 * Preceeding the command should be the hex value 0xAF
 * 
 * Following the command should be a 64-bit hash of the
 * 	last command
 *
 * Following the hash should be the value 0xFA
 *
 * Following the value 0xFA should be the start of the packet
 *
 * Finally, following the packet should be the value 0xAF
 *
 * The total layout looks as follows
 *
 * +-----------+---------+---------+-----------+--------+-----------+
 * | Hex Value | Command | Command | Hex Value | Packet | Hex Value |
 * |  0xAF     |         |   Hash  |   0xFA    |		|	0xAF    |
 * +-----------+---------+---------+-----------+--------+-----------+
 *
 * The value of all integer types should be in little-endian format
 */
struct boltsnap_command* read_command( int fd, struct boltsnap_command* cmd2,
										struct command_context* context ) {
	/* cmd is the command we will fill */
	struct boltsnap_command* cmd = cmd2;

	if( cmd == NULL ) {
		/* allocate the command on the heap if it is null */
		cmd = malloc( sizeof( struct boltsnap_command ) );
	}

	/* set it so that there are no errors */
	cmd->errno = 0;

	/* used as a handle for 8-bit values */
	uint8_t nextbyte;

	/* used to hold the 64-bit hash */
	uint64_t hash;
	uint64_t hash2;

	check_read( read( fd, &nextbyte, sizeof(uint8_t) ),
		"Error while reading next byte\n", NULL );

	/* Check to make sure the first byte of the command
		is 0xAF. If it is not, then we should just die now */
	check_malformed( nextbyte == 0xAF,
		"Malformed Input! Expected 0xAF at beginning of command\n",
		DELETE_IF_NEED );

	/* populate the struct */
	check_read2( read( fd, &cmd->version, sizeof( uint32_t ) ),
		"Error while reading command version!\n", DELETE_IF_NEED );

	check_read2( read( fd, &nextbyte, sizeof( uint8_t ) ),
		"Error while reading command type!\n",DELETE_IF_NEED );
	cmd->type = nextbyte;

	check_read2( read( fd, &cmd->extension, sizeof( uint32_t ) ),
		"Error while reading type extension",  DELETE_IF_NEED );

	check_read2( read( fd, &cmd->packet_size, sizeof( uint32_t ) ),
		"Error while reading packet size!\n", DELETE_IF_NEED );

	/* Check to see if the packet size is too big */
	check_malformed( cmd->packet_size <= MAX_PACKET_SIZE,
		"Malformed input! Packet size exceeded max packet sizei!\n",
		DELETE_IF_NEED );

	__iftrace( char buf[128] );
	__iftrace(
		__cptrace("Read Command: %s\n",
		boltsnap_command_print( buf, cmd )) );

	cmd->packet = NULL;

	/* read in the hash from the stream */
	check_read2( read( fd, &hash, sizeof( uint64_t ) ),
		"Error while reading hash!\n", DELETE_IF_NEED );

	/* Check to make the hash computed from this
		struct matches the hash sent across the stream.
		This is a kind of simple checksum */
	check_malformed( hash == (hash2 = boltsnap_command_hash( cmd ) ),
		"Malformed input! Computed hash value does not match sent hash value.\n",
		__cperror("Hash recieved: %lx; Hash expected: %lx\n", hash, hash2);
		DELETE_IF_NEED );

	/* read the next byte, this should be 0xFA */
	check_read2( read( fd, &nextbyte, sizeof(uint8_t) ),
		"Error while reading next byte!\n", DELETE_IF_NEED );

	/* the next byte should be 0xFA,
		if it is not, we need to bail */
	check_malformed( nextbyte == 0xFA,
		"Malformed input! Expected 0xFA after end of hash",
		DELETE_IF_NEED );


	/* Here we lookup the command to get a handle
		to the set of functions used to read the packet */
	const struct cmdtbl_entry* entry =
		lookup_command( context->lookup_table, cmd->type );
	
	/* set the table entry of the command */
	cmd->table_entry = entry;
	
	/* read the packet from the function in entry */
	void* packet = entry->read( cmd, fd );

	/* make sure the packet was truely read */
	check_malformed( cmd->errno == 0,
		"Malformed packet! entry->read flagged an error!\n",
		DELETE_IF_NEED );

	/* set the packet equal to
		what we read */
	cmd->packet = packet;

	return cmd;
}

#define CHECK_WRITE( a ) if( (a) == -1 ) { __cperror("Error on Write!\n"); };

int boltsnap_command_write( struct boltsnap_command* cmd, int fd,
	/* if the write function is NULL then the table one is used */
	int (*write_func)( struct boltsnap_command*, int ) ) {

	uint8_t handle = 0xAF;
	uint64_t hash;

	__cptrace("Writing to file descriptor: %d\n", fd); 

	__iftrace( char buf[128] );

	__iftrace(
		__cptrace("Writing Command: %s\n",
		boltsnap_command_print( buf, cmd )) );

	if( ! cmd ) {
		__cperror("cmd is null!\n");
		return -3;
	}
	
	/* write the first byte, which should be 0xAF */
	CHECK_WRITE( write( fd, &handle, sizeof(uint8_t) ) );
	
	/* write the command raw */
	CHECK_WRITE( write( fd, &cmd->version, sizeof( uint32_t ) ) );
	handle = (uint8_t) cmd->type;
	CHECK_WRITE( write( fd, &handle, sizeof(uint8_t) ) );
	CHECK_WRITE( write( fd, &cmd->extension, sizeof( uint32_t ) ) );
	CHECK_WRITE( write( fd, &cmd->packet_size, sizeof( uint32_t ) ) );
	/* done writing raw command */

	/* write the hash value */
	hash = boltsnap_command_hash( cmd );
	__cptrace( "Command Hash Tag: %lx\n", hash );
	CHECK_WRITE( write( fd, &hash, sizeof( uint64_t ) ) );

	/* write the hex value 0xFA */
	handle = 0xFA;
	CHECK_WRITE( write( fd, &handle, sizeof( uint8_t ) ) );
	
	if( write_func == NULL ) {
		write_func = cmd->table_entry->write;
	}

	if( write_func == NULL ) {
		__cperror( "Command table was null. Not able to write data!\n" );
		return -2;
	}

	/* writes the packet to the file descriptor */
	if( (hash = write_func( cmd, fd )) != 0 ) {
		return (int)(hash);
	}

	/* writes the terminating 0xAF */
	handle = 0xAF;
	check_write( write( fd, &handle, sizeof( uint8_t ) ),
		"Error on writing last byte 0xAF\n", 1 );

	return 0;
}

int boltsnap_command_destroy( struct boltsnap_command* cmd,
								struct command_context* context ) {
	if( cmd && cmd->packet ) {
		// only free the packet if it exists
		const struct cmdtbl_entry* entry =
			lookup_command( context->lookup_table, cmd->type );
		entry->destroy( cmd->packet );
		free(cmd->packet);
	}
	return 0;
}

uint64_t boltsnap_command_hash( struct boltsnap_command* cmd ) {
	uint64_t handle0 = 0;
	uint64_t handle1 = 0;
	uint64_t handle2 = 0;

	uint32_t version = cmd->version;
	uint32_t extension = cmd->extension;
	uint32_t packet_size = cmd->packet_size;

	// first byte of version goes to top
	handle0 |= (uint64_t)(version & 0x000000FF) << 48;
	handle0 |= (uint64_t)(version & 0x0000FF00) << 24 ;
	handle0 |= (uint64_t)(version & 0x00FF0000) ;
	handle0 |= (uint64_t)(version & 0xFF000000) >> 24 ;

	handle1 |= (uint64_t)(extension & 0xFF000000) << 24;
	handle1 |= (uint64_t)(extension & 0x00FF0000) << 16;
	handle1 |= (uint64_t)(extension & 0x0000FF00) << 8;
	handle1 |= (uint64_t)(extension & 0x000000FF) ;

	handle2 |= (uint64_t)(packet_size & 0x000000FF) << 56;
	handle2 |= (uint64_t)(packet_size & 0x0000FF00) << 32;
	handle2 |= (uint64_t)(packet_size & 0x00FF0000) << 8;
	handle2 |= (uint64_t)(packet_size & 0xFF000000) >> 16;

	handle0 += handle1 + handle2;
	handle2 = 0;

	uint8_t type = (uint8_t) ( cmd->type );
	while( type > 0 ) {
		handle2 |= type & 1;
		handle2 <<= 8;
		type >>= 1;
	}
	handle0 += handle2;
	
	return handle0;
}

struct info_t {
	struct boltsnap_command* cmd;
	struct command_context* context;
	callback_t callback;
};

#define HALT goto exit
pthread_mutex_t command_dispatch_mutex;
static void* command_dispatch_2( void* ptr ) {
	struct info_t *inf = (struct info_t*)ptr;

	if( inf == NULL ) {
		__cperror( "Info passed to command_dispatch_2 was NULL!\n" );
		pthread_exit(NULL);
	}


	struct command_context* context = inf->context;
	struct boltsnap_command* cmd = inf->cmd;

	__cptrace( "Dispatching command. Type=%d\n", cmd->type );
	const struct cmdtbl_entry* entry =
		lookup_command( context->lookup_table, cmd->type );
	
	__cptrace( "Running dispatch\n" );

	if( entry == NULL ) {
		/* The table entry is
			NULL so we cannot continue */
		__cperror( "No table entry for command type %d\n", cmd->type );
		/* I hate myself right now */
		HALT;
	}

	if( entry->dispatch == NULL ) {
		/* In this case, dispatch has not
			been implemented */
		__cperror( "Entry command not implemented for command type %d\n", cmd->type );
		HALT;
	}

	/* make sure that only one command
		is proccessed at one time */
	__cptrace("Creating mutex lock\n");
	pthread_mutex_lock( &command_dispatch_mutex );
	
	/* dispatch the command. This spot
		should be sychronized */
	int exit_code = entry->dispatch( cmd );
	
	/* release the lock so the next
		command can run */
	__cptrace("Releasing mutex lock\n");
	pthread_mutex_unlock( &command_dispatch_mutex );


	__cptrace( "Runnig callback.\n" );
	if( inf->callback ) {
		inf->callback( cmd, exit_code );
	}

exit:
	free(ptr);
	pthread_exit(NULL);
}

void command_dispatch( struct boltsnap_command* cmd,
						struct command_context* context,
						callback_t callback ) {
	
	if( ! context ) {
		__cperror( "Context NULL for command_dispatch\n" );
		pthread_exit( NULL );
	} else if ( ! cmd ) {
		__cperror( "Attempt to dispatch NULL command\n" );
		pthread_exit( NULL );
	} else if ( ! context->lookup_table ) {
		__cperror( "Context incomplete for command_dispatch\n" );
		pthread_exit( NULL );
	}

	struct info_t* info = malloc(sizeof(struct info_t));

	info->cmd = cmd;
	info->context = context;
	info->callback = callback;
	
	/* dispatch the command */
	command_dispatch_2( info );
}

char* boltsnap_command_print( char* buf, const struct boltsnap_command* cmd ) {
	sprintf(buf, "{%d %d %d %d}", cmd->version, cmd->type, cmd->extension, cmd->packet_size);
	return buf;
}
