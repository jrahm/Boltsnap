#ifndef PLAYTHREAD_H_
#define PLAYTHREAD_H_

#include <pthread.h>

#define RUNNING	0
#define PAUSED	1
#define STOPPED	2
#define READY	3
#define ERROR	4

#define playthread_running( playthread )       ( (playthread)->status == RUNNING )
#define playthread_stopped( playthread )       ( (playthread)->status == STOPPED )
#define playthread_error( playthread )         (((playthread)->status & 0xFF) == ERROR )
#define playthread_errorno( playthread )        ((playthread)->status >> 8)
#define playthread_set_errorno( playthread, a ) ((playthread)->status = (a << 8) | ((playthread)->status & 0xFF))
#define playthread_ready( playthread )         ( (playthread)->status == READY )
#define playthread_paused( playthread )        ( (playthread)->status == PAUSED )

/* struct that describes a thread
	that plays */
struct playthread {
	/* The file descriptor
		to write to this thread */
	int writefd;

	/* The file descriptor
		to read from this thread */
	int readfd;
	
	/* The pthread behind this
		playthread */
	pthread_t thread;
	
	/* mutex lock for this thread */
	pthread_mutex_t mutex;
	
	/* status of the play thread.
		e.g. running, paused or stopped */
	int status;
};

/* initializes the playthread.
	starts a separate thread and
	creates a pipe to allow other
	threads to communicate with it */
int playthread_init( struct playthread* playthread );

/* starts a play thread and returns the
	thread of the running daemon */
pthread_t playthread_start( struct playthread* playthread );

/* The main thread that processes playback
	and runs the pipes */
void* playthread_main_thread( void* );

/* Returns the write end of the communication
	stream */
int playthread_writefd();

/* Sends a signal to the play thread requesting
	it plays the file referenced by `filename` */
void playthread_request_play( struct playthread* playthread,
								const char* filename );

/* Flags for the playback to stop
	and blocks until the playing thread
	is terminated */
void playthread_stop_playback( struct playthread* );

/* Unlocks the playing thread
	so it can resume */
void playthread_resume_playback( struct playthread* );

/* Locks the playing thread
	so it suspends playback */
void playthread_pause_playback( struct playthread* );

#endif
