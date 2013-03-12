#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/types.h>

#include <ao/ao.h>
#include <pthread.h>
#include <fcntl.h>

#include "libavutil/mathematics.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

#include "playthread.h"
#include "globals.h"

#define EXIT {playthread->status = ERROR; free(filename); pthread_exit(NULL);} 

int playthread_init( struct playthread* playthread ) {
	int pipes[2];
	if( unlikely( pipe( pipes ) ) ) {
		fprintf(stderr, "There was a problem creating communication pipes on playthread_init\n");
		return -1;
	}

	playthread->readfd  = pipes[0];
	playthread->writefd = pipes[1];

	pthread_mutex_init( &playthread->mutex, NULL );


	playthread->status = READY;

	return 0;
}

pthread_t playthread_start( struct playthread* playthread ) {
	pthread_t thread;
	pthread_create( &thread, NULL, playthread_main_thread, playthread );
	return thread;
}	

struct playblob {
	struct playthread* playthread;
	char* filename;
};

/* plays and audio file. Blob is a pointer
	to the string */
static void* play_audiofile( void* blob ) {
		struct playblob* playblob = blob;

		struct playthread* playthread = playblob->playthread;
		char* filename = playblob->filename;

		playthread->status = RUNNING;

		/* Create the new format context */
		AVFormatContext* container = avformat_alloc_context();
		
		if( avformat_open_input( &container, filename, NULL, NULL ) < 0 ) {
			fprintf( stderr, "Could not open %s no such file or directory!\n", filename );
			EXIT;
		}

		if( avformat_find_stream_info(container, NULL) < 0 ) {
			fprintf( stderr, "Could not find file info for %s\n", filename );
			EXIT;
		}

		av_dump_format( container, 0, filename, 0 );
		
		int streamid = -1;

		size_t i, nstreams;
		nstreams = container->nb_streams;
		
		AVStream* stream;
		for( i = 0; i < nstreams; ++ i ) {
			stream = container->streams[i];

			if( stream->codec->codec_type == AVMEDIA_TYPE_AUDIO ) {
				streamid = i;
				break;
			}
		}

		if( streamid == -1 ) {
			fprintf( stderr, "Could not find audio stream for %s\n", filename );
			EXIT;
		}

		AVCodecContext* ctx = stream->codec;
		AVCodec* codec = avcodec_find_decoder( ctx->codec_id );

		if( codec == NULL ) {
			fprintf( stderr, "Could not find codec for %s\n", filename );
			EXIT;
		} else if( avcodec_open2( ctx, codec, NULL ) < 0 ) {
			fprintf( stderr, "Codec cannot be found for %s\n", filename );
			EXIT;
		}

		ao_initialize();

		int driver = ao_default_driver_id();

		ao_sample_format sformat;
		enum AVSampleFormat sfmt = ctx->sample_fmt;

		if( sfmt == AV_SAMPLE_FMT_U8 ) {
			printf( "U8\n" );
			sformat.bits = 8;
		} else if (sfmt == AV_SAMPLE_FMT_S16 ) {
			printf( "S18\n" );
			sformat.bits = 16;
		} else if ( sfmt == AV_SAMPLE_FMT_S32 ) {
			printf( "S32\n" );
			sformat.bits = 32;
		}

		sformat.channels = ctx->channels;
		sformat.rate = ctx->sample_rate;
		sformat.byte_format = AO_FMT_NATIVE;
		sformat.matrix = 0;

		ao_device* adevice = ao_open_live( driver, &sformat, NULL );

		AVPacket packet;
		av_init_packet( &packet );

		AVFrame* frame = avcodec_alloc_frame();
		int buffer_size = AVCODEC_MAX_AUDIO_FRAME_SIZE + FF_INPUT_BUFFER_PADDING_SIZE;

		uint8_t buffer[ buffer_size ];
		packet.data = buffer;
		packet.size = buffer_size;

		int frameFinished = 0;
		while( av_read_frame( container, &packet ) >= 0 && !playthread_stopped( playthread ) ) {
			// __cptrace( "Start of while loop. Try to lock mutex.\n" );
			// lock the thread. This is how the pause
			// mecanism is implemented
			pthread_mutex_lock( &playthread->mutex );
			// __cptrace( "Mutex lock aquired. Unlocking" );
			pthread_mutex_unlock( &playthread->mutex );

			if( packet.stream_index == streamid ) {
				// __cptrace( "Decoding audio.\n" );				
				avcodec_decode_audio4( ctx, frame, &frameFinished, &packet );
				
				if( frameFinished ) {
					// __cptrace( "Playing frame.\n" );
					ao_play( adevice, (char*) frame->extended_data[0], frame->linesize[0] );
				}
			}

		}

		avformat_close_input( &container );
		ao_shutdown();

		free(filename);
		pthread_exit(NULL);
}

void* playthread_main_thread( void* blob ) {
	struct playthread* playthread = blob;
	uint32_t len;
	char* name;

	/* register all devices */
	av_register_all();

	while( 1 ) {
		/* read in the length of the string */
		if( unlikely( read( playthread->readfd, &len, sizeof( uint32_t ) ) == 0 ) ) {
			fprintf( stderr, "Error reading filename\n" );
			continue;
		}
		
		name = malloc( len + 1 );
		if( unlikely( name == NULL || read( playthread->readfd, name, len ) == 0 ) ) {
			fprintf( stderr, "Error reading filename\n" );
			if( name != NULL ) {
				free( name );
			}
			continue;
		}

		name[len] = '\0';
		
		__cpinfo("Recieved string: %s\n", name);

		struct playblob blob={ playthread, name };

		pthread_create( &playthread->thread, NULL, play_audiofile, &blob );
	}
	
	return NULL;
}

/* Requests the thread to stop playback
	and then play a new mp3 file */
void playthread_request_play( struct playthread* playthread,
								const char* filename ) {
	playthread_stop_playback(playthread);

	uint32_t len = strlen( filename );
	int fd = playthread->writefd;

	check_write( write( fd, &len, sizeof( len ) ),
		"Error while writing filename length. playthread_request_play\n",);
	check_write( write( fd, filename, len ),
		"Error while writing filename. playthread_request_play\n",);
}

/* Stops and joins the playing thread */
void playthread_stop_playback(struct playthread* playthread) {
	__cptrace( "Stopping playback on pthread\n" );
	if( playthread_running( playthread ) ) {
		/* flag to the process
			that it's time to stop */
		playthread->status = STOPPED;

		/* unlock the mutext in
			case the playback is pausedd */
		pthread_mutex_trylock( &playthread->mutex );
		pthread_mutex_unlock( &playthread->mutex );

		/* Wait for the thread to exit */
		pthread_join( playthread->thread, NULL );
		
		/* Allow the next playback
			to play */
		playthread->status = READY;
	}
}

void playthread_resume_playback( struct playthread* playthread ) {
	__cptrace( "Resuming playback releasing lock on pthread.\n" );
	/* unlock the mutex that is blocking
		the play thread if it is paused */
	if( playthread_paused( playthread ) ) {
		pthread_mutex_unlock( &playthread->mutex );
	}
	__ifdebug( else { __cpdebug( "The thread was not paused."
									" The status was instead %d\n",
										playthread->status); } );
}

void playthread_pause_playback( struct playthread* playthread ) {
	__cptrace( "Pausing playback. Locking mutex on thread.\n" );
	/* try to lock the mutex so the playing thread suspends */
	while( ! pthread_mutex_trylock( &playthread->mutex ) );
	
	/* set the thread to paused */
	playthread->status = PAUSED;
}
