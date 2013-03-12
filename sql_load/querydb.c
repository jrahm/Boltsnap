#include "mediaload.h"

#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include <jansson.h>
#include <sqlite3.h>

#define BOLT_QUERY_ENV	"BOLT_QUERY"
#define DIRECTORY_ID	"directory-id"

#define JOIN_QUERY_FMT "SELECT f."FILE_INFO_OID_FIELD",f."FILE_INFO_FILEBASENAME_FIELD",f."FILE_INFO_TYPE_FIELD\
						",m."MP3_INFO_OID_FIELD",m."MP3_INFO_TITLE_FIELD",m."MP3_INFO_ARTIST_FIELD",m."MP3_INFO_ALBUM_FIELD" FROM "\
						FILE_INFO_TABLE" AS f JOIN "MP3_INFO_TABLE" AS m ON f."FILE_INFO_MEDIA_OID_FIELD"=m."MP3_INFO_OID_FIELD\
						" WHERE f.directory_oid=%ld"

json_t* dir_info_to_json( const struct dir_info* info ) {
	json_t* ret = json_object();

	json_object_set( ret, DIR_INFO_OID_FIELD, json_integer( info->oid ) );
	json_object_set( ret, DIR_INFO_DIRBASENAME_FIELD, json_string( info->dirbasename ) );
	json_object_set( ret, DIR_INFO_PARENT_OID_FIELD, json_integer( info->parent_oid ) );
	
	return ret;
}

json_t* dirs_to_json_del( struct dir_info_list* lst ) {
	struct dir_info_list* next;

	json_t* retarr = json_array();

	while( lst ) {
		next = lst->next;
		json_array_append( retarr, dir_info_to_json( lst->data ) );
		dir_info_delete( lst->data );
		free( lst->data );
		free(lst);
		lst = next;
	}

	return retarr;
}

#define FILE_PREFIX "file_"
#define MEDIA_PREFIX "media_"

json_t* files_to_json( sqlite3* db, int64_t diroid ) {
	sqlite3_stmt* stmt;
	char full_query[ sizeof(JOIN_QUERY_FMT) + 20 ];
	snprintf( full_query, sizeof( full_query ), JOIN_QUERY_FMT, (long) diroid );
	const char* tail;
	const unsigned char* __sqlstr2;

	/* The return array */
	json_t* retarr = json_array();

	if( sqlite3_prepare_v2( db, full_query, -1, &stmt, &tail ) ) {
		fprintf(stderr, "Can't retrieve data: %s\n", sqlite3_errmsg(db));
		return NULL;	
	}

	while( sqlite3_step( stmt ) == SQLITE_ROW ) {
		json_t* obj = json_object();

		int64_t sqlint = sqlite3_column_int( stmt, 0 );

		json_object_set( obj, FILE_PREFIX FILE_INFO_OID_FIELD, json_integer( sqlint ) );
		
		__sqlstr2 = sqlite3_column_text( stmt, 1 );
		json_object_set( obj, FILE_PREFIX FILE_INFO_FILEBASENAME_FIELD, json_string( (char*)__sqlstr2 ) );

		sqlint = sqlite3_column_int( stmt, 2 );
		json_object_set( obj, FILE_PREFIX FILE_INFO_TYPE_FIELD, json_integer( sqlint ) );

		sqlint = sqlite3_column_int( stmt, 3 );
		json_object_set( obj, MEDIA_PREFIX MP3_INFO_OID_FIELD, json_integer( sqlint ) );

		__sqlstr2 = sqlite3_column_text( stmt, 4 );
		json_object_set( obj, MEDIA_PREFIX MP3_INFO_TITLE_FIELD, json_string( (char*)__sqlstr2 ) );

		__sqlstr2 = sqlite3_column_text( stmt, 5 );
		json_object_set( obj, MEDIA_PREFIX MP3_INFO_ARTIST_FIELD, json_string( (char*)__sqlstr2 ) );

		__sqlstr2 = sqlite3_column_text( stmt, 6 );
		json_object_set( obj, MEDIA_PREFIX MP3_INFO_ALBUM_FIELD, json_string( (char*)__sqlstr2 ) );

		json_array_append( retarr, obj );
	}

	sqlite3_finalize( stmt );
	
	return retarr;
}

int main( int argc, char** argv, char** envp ) {
	char* bolt_query = getenv( BOLT_QUERY_ENV );
	int64_t diroid;

	json_t* root;
	json_error_t error;
	json_t* directoryid;

	struct dir_info_query dirquery;
	
	sqlite3* handle;

	if( sqlite3_open( MEDIA_DB, &handle ) ) {
		fprintf(stderr, "Error opening database!\n");
		return 2;
	}

	/* If the query is not in the
		environment, then try the arguments */
	if( ! bolt_query ) {
		if( argv[1] ) {
			bolt_query = argv[1];
		} else {
			fprintf( stderr, "Need to provide a json object either through first"
							 " argument or environment variable " BOLT_QUERY_ENV "\n");
			return 1;
		}
	}

	root = json_loads( bolt_query,0, &error );
	
	if( ! root ) {
		fprintf( stderr, "error: on line %d: %s\n", error.line, error.text );
		return 1;
	}

	directoryid = json_object_get(root, DIRECTORY_ID );
	
	if(!json_is_integer( directoryid )) {
		fprintf(stderr, DIRECTORY_ID " is not an integer!\n");
		return 1;
	}

	diroid = json_integer_value( directoryid );

	init_dir_info_query( &dirquery );
	dirquery.parent_oid = diroid;

	struct dir_info_list* dirres = get_dir_info_from_db1( handle, &dirquery, 0);
	json_t* dirs = dirs_to_json_del( dirres );
	json_t* files = files_to_json( handle, diroid );

	json_t* final_ret = json_object();

	json_object_set( final_ret, "dirs", dirs );
	json_object_set( final_ret, "files", files );

	char* str = json_dumps( final_ret, 0 );
	printf( "%s\n", str );
	free( str );

	return 1;
}
