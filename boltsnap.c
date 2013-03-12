#include "boltsnap.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <pthread.h>
#include "debug.h"

/* struct which is a tuple of
	connection info and a handler */
struct __connection_and_handler {
	boltsnap_connect_handler handler;
	struct boltsnap_connection conn;
};

/* The thread mutex used for synchronization in __start_serv_unix */
pthread_mutex_t server_connection_mutex;

/* The connection used in __start_serv_unix */
struct __connection_and_handler boltsnap_conn;

/* Wrapper function to call the connection
	handler */
void* __server_connection_dispatch( void* conn ) {
	struct __connection_and_handler* cnh = conn;
	void* ret = cnh->handler( cnh->conn );
	
	/* exit the pthread library */
	pthread_exit( ret );
	return ret;
}

/* creates a new server listening for
	connections on a unix domain socket */
int __start_serv_unix( struct boltsnap_config* conf,
						boltsnap_connect_handler handler, int sock ) {
	int rc, len;
	struct sockaddr_un local, remote;
	/* initialize the mutex */
	pthread_mutex_init( &server_connection_mutex, NULL );
	
	/* link the socket to a folder and
		call it a unix socket */
	local.sun_family = AF_UNIX;
	strcpy( local.sun_path, conf->sockpath );
	unlink( local.sun_path );

	len = strlen( local.sun_path ) + sizeof(local.sun_family);
	
	__cpdebug( "Creating socket on path: %s (len=%d)\n",
										local.sun_path, len );

	/* attempt to bind to the address */
	if( (rc = bind(sock, (struct sockaddr*)&local, len)) == -1 ) {
		fprintf(stderr, "Error on bind!\n");
		return rc;
	}

	/* listen to the socket */
	if( (rc = listen(sock, conf->backlog)) == -1 ) {
		fprintf( stderr, "Error on listen()\n" );
		return rc;
	}

	/* main loop */
	while( 1 ) {
		socklen_t t = sizeof(remote);
		
		if( (rc = accept(sock, (struct sockaddr *)&remote, &t)) == -1 ) {
			fprintf( stderr, "Error on accept()\n" );
			return rc;
		}
		
		/* make sure we don't change any values while dispatching */
		pthread_mutex_lock( &server_connection_mutex );
		boltsnap_conn.conn.type = AF_UNIX;
		boltsnap_conn.conn.fd = rc;
		boltsnap_conn.handler = handler;
		pthread_mutex_unlock( &server_connection_mutex );
		
		/* create a new thread that will handler the new connection */
		pthread_t thread;
		pthread_create( &thread, NULL, __server_connection_dispatch, &boltsnap_conn );
	}

	return 0;
}

/* Function to start the Boltsnap server
	most of the logic is in the helper methods */
int boltsnap_start_server( struct boltsnap_config* conf,
							boltsnap_connect_handler handler ) {
	int sock;

	sock = socket( conf->type, SOCK_STREAM, 0 );

	switch( conf->type ) {
		case AF_UNIX:
			return __start_serv_unix( conf, handler, sock );

		case AF_INET:
			fprintf(stderr, "AF_INET Settings not implemented! (Yet)\n");
			return -1;
	}

	return -2;
}

int __connect_unix( struct boltsnap_config* conf ) {
	int s, len;
	struct sockaddr_un remote;

	/* Create a new socket with the
		corrent type */
	s = socket( AF_UNIX, SOCK_STREAM, 0 );
	if( s == -1 ) {
		fprintf( stderr, "Error on socket()\n" );
		return s;
	}

	/* create the remote description */
	remote.sun_family = AF_UNIX;

	/* copy the path in */
	strcpy( remote.sun_path, conf->sockpath );
	len = strlen( remote.sun_path ) + sizeof( remote.sun_family );
	
	/* try to connect */
	if( connect( s, (struct sockaddr *)&remote, len ) == -1 ) {
		fprintf( stderr, "Error on connect()\n" );
		return -1;
	}

	return s;
}

int boltsnap_connect( struct boltsnap_config* conf ) {
	switch( conf->type ) {
	case AF_UNIX:
		return __connect_unix( conf );
	case AF_INET:
			fprintf(stderr, "AF_INET Settings not implemented! (Yet)\n");
			return -1;
	}

	return -2;
}
