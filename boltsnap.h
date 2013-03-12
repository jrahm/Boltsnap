#ifndef BOLTSNAP_H_
#define BOLTSNAP_H_

/* a struct that represents a connection
	to boltsnap */
struct boltsnap_connection {
	/* The file descriptor used
		to read and write from this
		connection */
	int fd;
	
	/* The type of the connection
		used */
	int type;
};

/* type used to handle incomming connections
	to the server. The void* is a pointer to
	a struct boltsnap_connection */
typedef void *(*boltsnap_connect_handler)( struct boltsnap_connection conn );

/* Configuration for the boltsnap server and
	client. This contains information about
	how the server should be configured */
struct boltsnap_config {
	/* The port the server is listening
		on. If this is an invalid port
		(port < 0 || port > 65536) then
		the default port is used */
	int port;

	/* The type of socket to
		create. e.g. AF_UNIX or
			AF_INET */
	int type;
	
	/* The maximum number of pending
		connections to the server */
	int backlog;

	union {
		/* Hostname is used for internet
			sockets */
		char* hostname;

		/* sockpath is used for unix
			domain sockets */
		char* sockpath;
	};
};

/* Opens a connection accorting to the
	boltsnap_config struct. Each time a
	new connection is recieved, a new thread
	is created which runs the handler */
int boltsnap_start_server( struct boltsnap_config*,
	boltsnap_connect_handler handler );

/* Attempts to connect to a boltsnap
	server and returns a valid file descriptor
	on success, or -1 on error */
int boltsnap_connect( struct boltsnap_config* );

#endif
