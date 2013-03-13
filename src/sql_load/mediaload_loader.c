#include "mediaload.h"

#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <sqlite3.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "libavformat/avformat.h"

const char* APPROVED_TYPES[] = { ".mp3" };

int filename_of_approved_type( const char* filename ) {
	size_t i = 0;
	const char* end = filename + strlen(filename);
	for( i = 0;i < sizeof(APPROVED_TYPES)/sizeof(const char*); ++ i ) {
		if( strcmp( end-strlen(APPROVED_TYPES[i]), APPROVED_TYPES[i] ) == 0 ) {
			return 1;
		}
	}

	return 0;
}

/* Context used to load data *//*
struct load_context {
	char* directory;
	sqlite3* db;
}; */

/* returns a pointer to the actual
	part of the string that contains
	the basename. */
static const char* mybasename( const char* name ) {
	size_t len = strlen(name);
	const char* cur = name + (len-1);
	while( *cur == '/' && cur != name ) -- cur;
	while( *cur != '/' && cur != name ) -- cur;
	if( cur != name ) ++ cur;
	return cur;
}

static int get_dir_info( const char* dir, struct dir_info* inf,
							int64_t parentid, struct load_context* context ) {
	char* fullpath = realpath( dir, NULL );
	char* basename = strdup( mybasename( dir ) );
	char* cur = basename;
	
	while( *cur != '/' && *cur != '\0' ) ++ cur;
	*cur = 0x0; /* cut off any trailing '/'s */
	
	/* First check to see if this
		dir info exists in the database */
	struct dir_info_query query = {-1, basename, fullpath, parentid};
	struct dir_info_query*tmp = &query;
	struct dir_info_list* lst = get_dir_info_from_db( context->db, &tmp, 1, 1 );

	if( lst != NULL ) {
		printf("There is already a dir in the db named '%s'", fullpath);
		/* there is already an entry
			in the database */
		*inf = *lst->data;

		free(lst->data);
		free(lst);
		free( basename );
		free( fullpath );

		return 0;
	};

	inf->dirbasename = basename;
	inf->dirpath = fullpath;
	inf->parent_oid = parentid;
	
	/* insert the data */
	insert_dir_info( context->db, inf );
	
	/* we have created a new entry */
	return 1;
}

static int64_t insert_media_info_for_filename( const char* file, 
												struct load_context* context ) {
	struct mp3_info inf;
	printf("\t\n\nAttempting to open media file %s\n", file );
	AVFormatContext* container = avformat_alloc_context();
	if( avformat_open_input( &container, file, NULL, NULL ) < 0 ) {
		fprintf( stderr, "Could not open file %s\n", file );
		return -1;
	} if( avformat_find_stream_info(container, NULL) < 0 ) {
		fprintf(stderr, "Could not find file info for %s\n", file);
		//avformat_close_input_file(container);
		avformat_free_context( container );
		return -1;
	}
	printf("\tSuccessfully opened media file %s!\n", file);

	inf.oid = -1;

	AVDictionaryEntry* entry = av_dict_get( container->metadata, "title", NULL, 0);
	if( entry ) {
		inf.title = strdup(entry->value);
	} else {
		inf.title = strdup( mybasename(file) );
	}
	entry = av_dict_get( container->metadata, "artist", NULL, 0 );
	if( entry ) {
		inf.artist = strdup( entry->value );
	} else {
		inf.artist = strdup( "Unknown Artist" );
	}
	entry = av_dict_get( container->metadata, "album", NULL, 0 );
	if( entry ) {
		inf.album = strdup( entry->value );
	} else {
		inf.album = strdup( "Unknown Album" );
	}
	
	inf.metadata = NULL;
	inf.metadata_blob_len =  0;

	printf("Inserting mp3 info\n");
	int64_t ret = insert_mp3_info( context->db, &inf );

	/* some teardown */
	mp3_info_delete( &inf );
	
	//avformat_close_input_file(container);
	avformat_free_context( container );

	return ret;
}

static int get_file_info( const char* file, struct file_info* inf,
							int64_t parent_dir_id, struct load_context* context ) {
	printf("\n\nGetting file info for file: %s\n", file);

	if( !filename_of_approved_type( file ) ) {
		fprintf(stderr,"File %s is not an approved media file.\n",file);
		return -1;
	}

	char* fullpath = realpath( file, NULL );
	char* basename = strdup( mybasename( file ) );
	char* cur = basename;
	while( *cur != '/' && *cur != '\0' ) ++ cur;
	*cur = 0x0; /* cut off any trailing '/'s */

	struct file_info_query query = {-1,-1,basename,fullpath,parent_dir_id,-1};
	struct file_info_query*tmp = &query;
	struct file_info_list* lst = get_file_info_from_db( context->db, &tmp, 1, 1 );
	
	if( lst != NULL ) {
		printf("The File %s already exists!\n", fullpath );
		*inf = *lst->data;

		free( lst->data );
		free( lst );
		free(fullpath);
		free(basename);

		return 0;
	}

	inf->filebasename = basename;
	inf->filepath = fullpath;
	// TODO change this
	inf->type = MP3;
	inf->directory_oid = parent_dir_id;
	inf->media_oid = insert_media_info_for_filename(fullpath, context);	
	
	if( inf->media_oid >= 0 ) {
		printf("Inserting File: %s\n", fullpath ); 
		insert_file_info( context->db, inf );
	} else {
		printf("Error inserting media info for file: %s\n", fullpath);
	}

	return 0;
}

static int load_dir_rec( const char* dir, int64_t parent_dir_oid,
						struct load_context* context ) {
	struct stat s;
	size_t len = strlen(dir);
	size_t len2 = 0;
	struct dir_info dirinf;
	char* next = NULL;

	if( get_dir_info( dir, &dirinf, parent_dir_oid, context ) ) {
		printf("Directory '%s' has been inserted\n", dir );
	}

	DIR* handle = opendir( dir );
	struct dirent* ent;

	while( handle != NULL && (ent = readdir(handle)) != NULL ) {
		if( strcmp(ent->d_name, ".") != 0 &&
			strcmp(ent->d_name, "..") != 0 ) {
			len2 = len + strlen( ent->d_name ) + 16;
			next = realloc( next, len2 );

			snprintf(next, len2, "%s/%s", dir, ent->d_name);
			stat( next, &s );

			if( S_ISDIR( s.st_mode ) ) {
				load_dir_rec( next, dirinf.oid, context );
			} else {
				struct file_info inf;
				if( get_file_info( next, &inf, dirinf.oid, context ) >= 0 ) {
					file_info_delete( &inf );
				}
			}
		}
	}

	dir_info_delete( &dirinf );
	closedir( handle );
	free(next);

	return 0;
}

int load_directory_root( struct load_context* context ) {
	av_register_all();
	return load_dir_rec( context->directory, 0, context );
}

