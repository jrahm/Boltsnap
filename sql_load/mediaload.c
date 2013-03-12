#include "mediaload.h"

#include <sqlite3.h>
#include <string.h>
#include <stdlib.h>

#define ESCAPE_CHARS "\'"

#define to_hex( b ) ((b) < 10) ? ((b) + 0x30) : ((b)  + 0x57)

struct file_info_list* get_file_info_from_db1( sqlite3* db,
						struct file_info_query* query, size_t limit ) {
	return get_file_info_from_db( db, &query, 1, limit );
}

struct mp3_info_list* get_mp3_info_from_db1( sqlite3* db,
						struct mp3_info_query* query, size_t limit ) {
	return get_mp3_info_from_db( db, &query, 1, limit );
}

struct dir_info_list* get_dir_info_from_db1( sqlite3* db,
						struct dir_info_query* query, size_t limit ) {
	return get_dir_info_from_db( db, &query, 1, limit );
}

void init_file_info_query( struct file_info_query* query ) {
	query->oid = -1;
	query->type = -1;
	query->filebasename = NULL;
	query->filepath = NULL;
	query->directory_oid = -1;
	query->media_oid = -1;
}

void init_dir_info_query( struct dir_info_query* query ) {
	query->oid = -1;
	query->dirbasename = NULL;
	query->dirpath = NULL;
	query->parent_oid = -1;
}

void init_mp3_info_query( struct mp3_info_query* query ) {
	query->oid = -1;
	query->title = NULL;
	query->artist = NULL;
	query->album = NULL;
}

void disintegrate_list( struct genlist* lst, void(*callback)(void* data) ) {
	struct genlist* next;
	while( lst ) {
		next = lst->next;
		
		if( callback ) {
			callback( lst->data );
		}

		free( lst );
		lst = next;
	}
}

void file_info_delete( struct file_info* inf ) {
	free( inf->filebasename );
	free( inf->filepath );
}

void mp3_info_delete( struct mp3_info* inf ) {
	free( inf->title );
	free( inf->artist);
	free( inf->album );
	free( inf->metadata );
}

void dir_info_delete( struct dir_info* inf ) {
	free( inf->dirbasename );
	free( inf->dirpath );
}


char* blob_to_sqlite_hex( const void* blob, size_t bloblen ) {
	const unsigned char *blob2 = blob;

	char* ret = malloc( (bloblen << 1) + 1 );
	char* cur = ret;
	size_t i;
	for( i = 0;i < bloblen; ++ i ) {
		unsigned b = blob2[i];
		cur[0] = to_hex( b >> 4 );
		cur[1] = to_hex( b & 0x0F );
		cur += 2;
	}
	*cur = '\0';
	return ret;
}

/* a helper function which copies the string `thing` into
	the buffer buf up to *len bytes. It then returns a pointer
	to (buf + strlen(thing)) and automatically updates len to be
	*len = *len - strlen(thing). */
char* mystrcpy( char* buf, const char* thing, size_t* len ) {
	size_t l = *len;
	for( ; ((*buf) = (*thing)) != 0 && len > 0; ++ buf, ++ thing, --l );
	return buf;
}

/* returns a version of `str` which
	has been escaped to fit inside of
	a sqlite string */
char* escape_string( const char* str ) {
	if( str == NULL ) {
		return strdup( "NULL" );
	}
	size_t len = 0;
	size_t escapes = 1; // '\0'
	size_t cur = 0;

	for( ; str[len] != '\0'; ++ len ) {
		if( strchr( ESCAPE_CHARS, str[len] ) ) {
			++ escapes;
		}
	}
	char* ret = malloc( len + escapes );
	
	for( len = 0; str[len] != '\0'; ++ len ) {
		if( strchr( ESCAPE_CHARS, str[len] ) ) {
			ret[cur++] = '\'';
		}
		ret[cur++] = str[len];
	}
	ret[cur] = '\0';

	return ret;
}

#define CREATE_TBL "CREATE TABLE IF NOT EXISTS "

/* query to create dir info table */
#define CREATE_DIR_INFO_TABLE 	\
		CREATE_TBL DIR_INFO_TABLE "("DIR_INFO_DIRBASENAME_FIELD" TEXT, " \
		DIR_INFO_DIRPATH_FIELD" TEXT, "DIR_INFO_PARENT_OID_FIELD" INTEGER)"

/* query to insert mp3 info table */
#define CREATE_MP3_INFO_TABLE \
		CREATE_TBL MP3_INFO_TABLE "("MP3_INFO_TITLE_FIELD" TEXT, " \
		MP3_INFO_ARTIST_FIELD" TEXT, "MP3_INFO_ALBUM_FIELD" TEXT, " \
		MP3_INFO_METADATA_FIELD" BLOB)"

#define CREATE_FILE_INFO_TABLE \
		CREATE_TBL FILE_INFO_TABLE "("FILE_INFO_TYPE_FIELD" INTEGER,"\
		FILE_INFO_FILEBASENAME_FIELD" TEXT, "FILE_INFO_FILEPATH_FIELD" TEXT, "\
		FILE_INFO_DIRECTORY_OID_FIELD" INTEGER, "FILE_INFO_MEDIA_OID_FIELD" INTEGER)"

/* Setup the database pointed to by handle */
void setup_database( sqlite3* handle ) {
	char* err;
	
	/* The array of queries to to execute */
	const char* queries[] = {CREATE_DIR_INFO_TABLE,
	CREATE_MP3_INFO_TABLE,CREATE_FILE_INFO_TABLE};

	size_t nqueries = sizeof( queries ) / sizeof( const char * );
	size_t i;

	/* iterate through the queries */
	for( i = 0; i < nqueries; ++ i ) {
			const char* query = queries[i];
			printf( "Executing: %s\n", query );

			if( sqlite3_exec( handle, query , 0,0,&err ) ) {
				fprintf(stderr, "SQL Error: %s\n", err);
				sqlite3_free(err);
			}
	}

}


