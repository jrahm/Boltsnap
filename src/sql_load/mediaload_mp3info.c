#include <stdlib.h>
#include <string.h>

#include "mediaload.h"


char* mp3_info_to_string( const struct mp3_info* inf, char* buf, size_t len ) {
	char* escape;
	snprintf( buf, len, "struct mp3_info{oid=%ld, title='%s', artist='%s', album='%s', metadata=X'%s'}",
		LONG_INT_CAST inf->oid, inf->title, inf->artist, inf->album, (escape=blob_to_sqlite_hex(inf->metadata,inf->metadata_blob_len)));
	free(escape);
	return buf;
}

struct str_entry { const char* key; const char* val; };

static size_t estimate_len( struct mp3_info_query* query ) {
	return	strlen( query->title ) +
			strlen( query->artist) +
			strlen( query->album ) + 128;
	
}

/* generates a query suffix for one mp3 info query */
static char* generate_query_suffix( struct mp3_info_query* query ) {
	/* estimate the length of the query suffix */
	size_t est_len = estimate_len( query );
	char* ret = malloc( est_len );
	size_t len = est_len;
	char* cursor = ret;

	char oidbuf[20];
	snprintf( oidbuf, 20, "%ld", LONG_INT_CAST query->oid );

	struct str_entry filters[] = {
		{ MP3_INFO_OID_FIELD, query->oid == -1 ? NULL : oidbuf },
		{ MP3_INFO_TITLE_FIELD, query->title },
		{ MP3_INFO_ARTIST_FIELD, query->artist },
		{ MP3_INFO_ALBUM_FIELD, query->album },
	};

	size_t i;
	size_t nfilters = sizeof( filters ) / sizeof( struct str_entry );
	int do_and = 0;

	for( i = 0;i < nfilters; ++ i ) {
		if( filters[i].val != NULL ) {
			/* There is stuff for this filter */
			if( do_and ) {
				cursor = mystrcpy( cursor, " AND ", &len );
			} else {
				do_and = 1;
			}

			cursor = mystrcpy( cursor, filters[i].key, &len );
			cursor = mystrcpy( cursor, " LIKE '", &len );
			cursor = mystrcpy( cursor, filters[i].val, &len );
			cursor = mystrcpy( cursor, "'", &len );

		}
	}
	
	if( len > 0 ) {
		*cursor = '\0';
	}
	return ret;
}

static char* generate_full_query( struct mp3_info_query** queries, size_t nqueries, size_t limit ) {
	size_t total_est = 0;
	size_t i = 0;
	char* suf;
	for( ; i < nqueries; ++ i ) {
		total_est += estimate_len( queries[i] );
	}
	total_est += nqueries * 8; /* room for " OR " and "(" ")" */
	total_est += sizeof( SELECT_ALL_FROM MP3_INFO_TABLE " WHERE " );
	size_t len = total_est;

	char* ret = malloc( total_est );
	char* cursor = ret;
	cursor = mystrcpy( cursor, SELECT_ALL_FROM MP3_INFO_TABLE " WHERE (", &len);
	for( i = 0; i < nqueries; ++ i ) {
		suf = generate_query_suffix( queries[i] );
		cursor = mystrcpy( cursor, "(", &len );
		cursor = mystrcpy( cursor, suf, &len );
		cursor = mystrcpy( cursor, ")", &len );
		if( i < nqueries - 1 ) {
			cursor = mystrcpy( cursor, " OR ", &len );
		}
		free(suf);
	}
	cursor = mystrcpy( cursor, ")", &len );

	if( limit > 0 ) {
		char buf[30];
		snprintf(buf, 30, " LIMIT %lu", LONG_INT_CAST limit );
		cursor = mystrcpy( cursor, buf, &len );
	}

	if(len > 0) {
		*cursor = '\0';
	}
	return ret;
}

/* creates a new dir_info from that statement passed */
static inline struct mp3_info* create_from_stmt( sqlite3_stmt* res ) {
	struct mp3_info* tmp = malloc(sizeof(struct mp3_info));

	memset( tmp, 0, sizeof(struct dir_info));

	/* set the data */
	tmp->oid = sqlite3_column_int ( res, 0 );
	/* have to use strdup so the pointers
		are not deleted on finalize */
	tmp->title  = strdup( (char*)sqlite3_column_text( res, 1 ) );
	tmp->artist = strdup( (char*)sqlite3_column_text( res, 2 ) );
	tmp->album  = strdup( (char*)sqlite3_column_text( res, 3 ) );
	
	const void* blob = sqlite3_column_blob( res, 4 );
	size_t blob_bytes = sqlite3_column_bytes( res, 4 );
	tmp->metadata = malloc( tmp->metadata_blob_len = blob_bytes );
	memcpy( tmp->metadata, blob, blob_bytes );

	return tmp;	
}

struct mp3_info_list* get_mp3_info_from_db( sqlite3* db, struct mp3_info_query** query,
												size_t nqueries, size_t limit ) {
	char* full_query = generate_full_query( query, nqueries, limit );
	sqlite3_stmt* res;
	const char* tail;
	
	printf( "Executing Query: %s\n", full_query );

	/* prepare the statement */
	if( sqlite3_prepare_v2( db, full_query, -1, &res, &tail ) ) {
		fprintf(stderr, "Can't retrieve data: %s\n", sqlite3_errmsg(db));
		free(full_query);
		return NULL;
	}

	/* make build the list to return */
	struct mp3_info_list* lst=NULL;
	struct mp3_info_list** node=&lst;
	
	while( sqlite3_step(res) == SQLITE_ROW ) {
		struct mp3_info_list* tmp = malloc( sizeof( struct file_info_list ) );
		
		tmp->data = create_from_stmt( res );
		tmp->next = NULL;

		*node=tmp;
		node = &tmp->next;
	}
	
	/* delete the query */
	free(full_query);

	/* done with stuff */
	sqlite3_finalize( res );
	return lst;

}
/* Inserts an mp3 info into the sqlite3 database pointed
	to by handle */
int64_t insert_mp3_info( sqlite3* handle, struct mp3_info* inf ) {
	/* two handles to malloc'd strings
		needed to delete after insertion */
	char* escape1;
	char* escape2;
	char* escape3;
	char* escape4;

	/* a large buffer to store a query */
	size_t est_size = 
		strlen( inf->title ) +
		strlen( inf->artist ) +
		strlen( inf->album ) +
		inf->metadata_blob_len + 1024;
	
	char* query = malloc( est_size );
	/* initialize the query */
	snprintf( query, est_size, INSERT_INTO MP3_INFO_TABLE VALUES
				"('%s', '%s', '%s', X'%s')",
				(escape1 = escape_string(inf->title)),
				(escape2 = escape_string(inf->artist)),
				(escape3 = escape_string(inf->album)),
				(escape4 = blob_to_sqlite_hex( inf->metadata, inf->metadata_blob_len ) ));
	
	/* free the handles */
	free(escape1);
	free(escape2);
	free(escape3);
	free(escape4);

	printf( "Executing mp3 Query: %s\n", query );
	
	/* execute the queries */
	if( sqlite3_exec(handle,query,0,0,&escape1) ) {
		/* if control reaches here, there
			has been an error */
		fprintf( stderr, "SQL Error: %s\n", escape1 );
		fprintf( stderr, "Query: %s\n", query );
		free(query);
		return -1;
	}
	free(query);
	/* set the oid of the the inserted
		info */
	inf->oid = sqlite3_last_insert_rowid( handle );

	/* return the inserted id */
	return inf->oid;
}
