#include "mediaload.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* TODO Make this file and the others more concise, to use more code reuse */

char* file_info_to_string( const struct file_info* inf, char* buf, size_t len ) {
	snprintf( buf, len, "struct file_info{oid=%ld, type=%ld, filebasename='%s', filepath='%s', directory_oid='%ld', media_oid='%ld'}",
		LONG_INT_CAST inf->oid, LONG_INT_CAST inf->type, inf->filebasename, inf->filepath, LONG_INT_CAST inf->directory_oid, LONG_INT_CAST inf->media_oid);
	return buf;
}

size_t estimate_query_length( struct file_info_query* query ) {
	return strlen( query->filebasename ) +
			strlen( query->filepath ) + 
			256;
}

struct str_entry { char* key; char* val; };
static char* generate_query_suffix( struct file_info_query* query ) {
	size_t est_len = estimate_query_length( query );
	char* ret = malloc( est_len );
	size_t len = est_len;
	char* cursor = ret;

	char oidbuf[20];
	char typebuf[20];
	char diroidbuf[20];
	char mediaoidbuf[20];

	snprintf( oidbuf, 20, "%ld", LONG_INT_CAST query->oid );
	snprintf( typebuf, 20, "%ld", LONG_INT_CAST query->type );
	snprintf( diroidbuf, 20, "%ld", LONG_INT_CAST query->directory_oid );
	snprintf( mediaoidbuf, 20, "%ld", LONG_INT_CAST query->media_oid );

	struct str_entry filters[] = {
		{ FILE_INFO_OID_FIELD, query->oid == -1 ? NULL : oidbuf },
		{ FILE_INFO_TYPE_FIELD, query->type == -1 ? NULL : typebuf },
		{ FILE_INFO_FILEBASENAME_FIELD, query->filebasename },
		{ FILE_INFO_FILEPATH_FIELD, query->filepath },
		{ FILE_INFO_DIRECTORY_OID_FIELD, query->directory_oid == -1 ? NULL : diroidbuf },
		{ FILE_INFO_MEDIA_OID_FIELD, query->media_oid == -1 ? NULL : mediaoidbuf },
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

};

static char* generate_full_query( struct file_info_query** queries, size_t nqueries, size_t limit ) {
	size_t total_est = 0;
	size_t i = 0;
	char* suf;
	for( ; i < nqueries; ++ i ) {
		total_est += estimate_query_length( queries[i] );
	}
	total_est += nqueries * 8; /* room for " OR " and "(" ")" */
	total_est += sizeof( SELECT_ALL_FROM FILE_INFO_TABLE " WHERE " );
	size_t len = total_est;

	char* ret = malloc( total_est );
	char* cursor = ret;
	cursor = mystrcpy( cursor, SELECT_ALL_FROM FILE_INFO_TABLE " WHERE (", &len);
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

static inline struct file_info* create_from_stmt( sqlite3_stmt* res ) {
	struct file_info* tmp = malloc(sizeof(struct file_info));

	memset( tmp, 0, sizeof(struct dir_info));

	/* set the data */
	tmp->oid = sqlite3_column_int ( res, 0 );
	/* have to use strdup so the pointers
		are not deleted on finalize */
	tmp->type  = sqlite3_column_int( res, 1 );
	tmp->filebasename = strdup( (char*)sqlite3_column_text( res, 2 ) );
	tmp->filepath  = strdup( (char*)sqlite3_column_text( res, 3 ) );
	tmp->directory_oid = sqlite3_column_int( res, 4 );
	tmp->media_oid  = sqlite3_column_int( res, 5 );
	
	return tmp;	
}

struct file_info_list* get_file_info_from_db( sqlite3* db, struct file_info_query** query,
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
	struct file_info_list* lst=NULL;
	struct file_info_list** node=&lst;
	
	while( sqlite3_step(res) == SQLITE_ROW ) {
		struct file_info_list* tmp = malloc( sizeof( struct file_info_list ) );
		
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
int64_t insert_file_info( sqlite3* handle, struct file_info* inf ) {
	/* two handles to malloc'd strings
		needed to delete after insertion */
	char* escape1 = NULL;
	char* escape2 = NULL;

	/* a large buffer to store a query */
	size_t est_size = 
		strlen( inf->filepath ) +
		strlen( inf->filebasename ) +
		1024;
	

	char* query = malloc( est_size );
	/* initialize the query */
	snprintf(
	query, est_size, INSERT_INTO FILE_INFO_TABLE VALUES
				"(%ld, '%s', '%s', %ld, %ld)",
				(long)inf->type,
				(escape1 = escape_string(inf->filebasename)),
				(escape2 = escape_string(inf->filepath)),
				(long)inf->directory_oid,
				(long)inf->media_oid );
	
	/* free the handles */
	free(escape1);
	free(escape2);

	printf( "Executing file Query: %s\n", query );
	
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
