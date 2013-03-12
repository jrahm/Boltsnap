#include "mediaload.h"

#include <sqlite3.h>
#include <string.h>
#include <stdlib.h>

#define ESCAPE_CHARS "\'"


/* Writes a string represenation of a dir_info to
	a character buffer and returns a pointer to that char buffer */
char* dir_info_to_string( const struct dir_info* inf, char* buf, size_t len ) {
	snprintf(buf, len, "struct dir_info{oid=%ld, dirbasename=\"%s\", dirpath=\"%s\", parent_oid=%ld}",
				LONG_INT_CAST inf->oid, inf->dirbasename, inf->dirpath, LONG_INT_CAST inf->parent_oid );
	return buf;
}

/* Inserts a dir info into the sqlite3 database pointed
	to by handle */
int64_t insert_dir_info( sqlite3* handle, struct dir_info* inf ) {
	/* two handles to malloc'd strings
		needed to delete after insertion */
	char* escape1;
	char* escape2;

	/* a large buffer to store a query */
	char query[1024];

	/* initialize the query */
	snprintf( query, 1024, "INSERT INTO " DIRECTORY_TABLE " VALUES(\'%s\', \'%s\', %ld)",
				(escape1 = escape_string(inf->dirbasename)), (escape2=escape_string(inf->dirpath)), LONG_INT_CAST inf->parent_oid );
	
	/* free the two handles */
	free(escape1);
	free(escape2);
	
	/* execute the queries */
	if( sqlite3_exec(handle,query,0,0,&escape1) ) {
		/* if control reaches here, there
			has been an error */
		fprintf( stderr, "SQL Error: %s\n", escape1 );
		fprintf( stderr, "Query: %s\n", query );	
		return -1;
	}
	
	/* set the oid of the the inserted
		info */
	inf->oid = sqlite3_last_insert_rowid( handle );

	/* return the inserted id */
	return inf->oid;
}

static char* generate_query_suffix( struct dir_info_query* query,
									char* buf, size_t* len  ) {
	char* cursor = buf;
	char tmp[128];
	char* escape;
	int do_and = 0;

	*cursor = '\0';

	if( query->oid != -1 ) {
		cursor = mystrcpy( cursor, DIR_INFO_OID_FIELD "=", len);
		snprintf( tmp, 128, "%ld", LONG_INT_CAST query->oid );
		cursor = mystrcpy( cursor, tmp, len );
		do_and = 1;
	}
	
	if( query->dirbasename != NULL ) {
		if( do_and ) {
			cursor = mystrcpy( cursor, " AND ", len );
		}
		cursor = mystrcpy( cursor, DIR_INFO_DIRBASENAME_FIELD " LIKE \'", len );
		escape = escape_string( query->dirbasename );
		
		cursor = mystrcpy( cursor, escape, len );
		cursor = mystrcpy( cursor, "\'", len );
	
		free(escape);
		do_and = 1;
	}

	if( query->dirpath != NULL ) {
		if( do_and ) {
			cursor = mystrcpy( cursor, " AND ", len );
		}
		
		cursor = mystrcpy( cursor, DIR_INFO_DIRPATH_FIELD " LIKE \'", len );
		escape = escape_string( query->dirpath );
		
		cursor = mystrcpy( cursor, escape, len );
		cursor = mystrcpy( cursor, "\'", len );

		free(escape);

		do_and = 1;
	}
	
	if( query->parent_oid != -1 ) {
		if( do_and ) {
			cursor = mystrcpy( cursor, " AND ", len );
		}
		cursor = mystrcpy( cursor, DIR_INFO_PARENT_OID_FIELD "=", len);
		snprintf( tmp, 128, "%ld", LONG_INT_CAST query->parent_oid );
		cursor = mystrcpy( cursor, tmp, len );
		do_and = 1;
	}

	return cursor;
}

/* creates a new dir_info from that statement passed */
static inline struct dir_info* create_from_stmt( sqlite3_stmt* res ) {
	struct dir_info* tmp = malloc(sizeof(struct dir_info));

	memset( tmp, 0, sizeof(struct dir_info));

	/* set the data */
	tmp->oid = 					sqlite3_column_int ( res, 0 );
	/* have to use strdup so the pointers
		are not deleted on finalize */
	tmp->dirbasename = 	strdup( (char*)sqlite3_column_text( res, 1 ) );
	tmp->dirpath = 		strdup( (char*)sqlite3_column_text( res, 2 ) );
	tmp->parent_oid = 	sqlite3_column_int ( res, 3 );

	return tmp;	
}

char* create_dir_info_query( struct dir_info_query** query,
								size_t nqueries, size_t limit ) {
	char* massive_query;
	char* cursor;
	size_t total = 0;
	size_t i = 0;
	
	/* iterate through the queries and
		add each of them to the length
		of the query (trying to estimate the size) */
	for( ; i < nqueries; ++ i ) {
		struct dir_info_query* tmp = query[i];
		
		if( tmp->dirbasename ) {
			total += strlen(tmp->dirbasename);
		} if( tmp->dirpath ) {
			total += strlen(tmp->dirpath);
		}
		
		/* for the length of the integers just in
			case they are 20 digits long each*/
		total += 40;
	}

	/* more buffer */
	total += 256;
	
	/* allocate the space for the query */
	massive_query = malloc( total );
	cursor = massive_query;
	
	/* start out withe the typical 
		select statement */
	cursor = mystrcpy( cursor, SELECT_ALL_FROM DIR_INFO_TABLE" WHERE (", &total );
	for( i = 0; i < nqueries; ++ i ) {
		/* for each of the queries add the suffix in
			its own parenthasis */
		cursor = mystrcpy( cursor, "(", &total );
		/* generate the query */
		cursor = generate_query_suffix( query[i], cursor, &total );
		/* bookend */
		cursor = mystrcpy( cursor, ")", &total );

		/* if we are not at the last query, add an OR */
		if( i < nqueries - 1 ) {
			cursor = mystrcpy( cursor, " OR ", &total );
		}
	}
	cursor = mystrcpy( cursor, ")", &total );
	
	if( limit > 0 ) {
		char buf[30];
		snprintf(buf, 30, " LIMIT %lu", LONG_INT_CAST limit );
		cursor = mystrcpy( cursor, buf, &total );
	}

	*cursor = '\0';
	
	return massive_query;
}

struct dir_info_list* get_dir_info_from_db( sqlite3* handle, struct dir_info_query** query,
												size_t nqueries, size_t limit ) {
	/* estimate size */
	
	/* generate the query string */
	char* massive_query = create_dir_info_query( query, nqueries, limit );
	const char* tail;
	
	/* the sqlite statement */
	sqlite3_stmt *res;


	printf("Generated query: %s\n", massive_query);
	
	/* prepare the statement */
	if( sqlite3_prepare_v2( handle, massive_query, -1, &res, &tail ) ) {
		fprintf(stderr, "Can't retrieve data: %s\n", sqlite3_errmsg(handle));
		free(massive_query);
		return NULL;
	}

	/* make build the list to return */
	struct dir_info_list* lst=NULL;
	struct dir_info_list** node=&lst;

	while( sqlite3_step(res) == SQLITE_ROW ) {
		struct dir_info_list* tmp = malloc( sizeof( struct dir_info_list ) );
		
		tmp->data = create_from_stmt( res );
		tmp->next = NULL;

		*node=tmp;
		node = &tmp->next;
	}
	
	/* delete the query */
	free(massive_query);

	/* done with stuff */
	sqlite3_finalize( res );
	return lst;
}

