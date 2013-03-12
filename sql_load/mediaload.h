#ifndef MEDIALOAD_H_
#define MEDIALOAD_H_

#include <sqlite3.h>
#include <inttypes.h>
#include <stdio.h>

#define DIRECTORY_TABLE "directories"

#define DIR_INFO_TABLE DIRECTORY_TABLE
#define DIR_INFO_OID_FIELD			"oid"
#define DIR_INFO_DIRBASENAME_FIELD	"dirbasename"
#define DIR_INFO_PARENT_OID_FIELD	"parent_oid"
#define DIR_INFO_DIRPATH_FIELD		"dirpath"

#define MP3_INFO_TABLE			"mp3s"
#define MP3_INFO_OID_FIELD		"oid"
#define MP3_INFO_TITLE_FIELD	"title"
#define MP3_INFO_ARTIST_FIELD	"artist"
#define MP3_INFO_ALBUM_FIELD	"album"
#define MP3_INFO_METADATA_FIELD	"metadata"

#define FILE_INFO_TABLE			"files"
#define FILE_INFO_OID_FIELD		"oid"
#define FILE_INFO_TYPE_FIELD	"media_type"
#define FILE_INFO_FILEBASENAME_FIELD	"basename"
#define FILE_INFO_FILEPATH_FIELD	"path"
#define FILE_INFO_DIRECTORY_OID_FIELD	"directory_oid"
#define FILE_INFO_MEDIA_OID_FIELD	"media_oid"

#define SELECT_ALL_FROM "SELECT oid,* FROM "
#define INSERT_INTO "INSERT INTO "
#define VALUES " VALUES"
#define MEDIA_DB "/usr/lib/boltsnap/mediadb.sqlite3"

#define LONG_INT_CAST (long int)

enum media_type{MP3};

/* info for an mp3 music file */
struct mp3_info {
	/* the row id of this mp3 info */
	int64_t oid;	
	/* The title of the song */
	char* title;
	/* The artist of the song */
	char* artist;
	/* The album name */
	char* album;
	/* a blob of metadata */
	void* metadata;
	/* size of the metadata blob */
	size_t metadata_blob_len;
};

struct mp3_info_query {
	int64_t oid;
	const char* title;
	const char* artist;
	const char* album;
};

/* The C Struct representation
	of a dir_info struct */
struct dir_info {
	/* The row id of this directory */
	int64_t oid;
	
	/* The base name of this directory */
	char* dirbasename;

	/* The path of this directory */
	char* dirpath;
	
	/* The parent of this directory */
	int64_t parent_oid;
	
	/* a pointer to the parent. If the
		parent is not inserted */
	struct dir_info* parent;
};

struct dir_info_query {
	/* The oid of the dir info, or -1 to match all */
	int64_t oid;

	/* The dirbasename of the entry
		to look up, or NULL to match all */
	char* dirbasename;

	/* The dirpath of the entry to
		look up, or NULL to match all */
	char* dirpath;

	/* The oid of the parent of the 
		entry to look up, or NULL to match all */
	int64_t parent_oid;
};

struct file_info {
	/* The row id of this file info */
	int64_t oid;

	/* The type of the media in this file */
	enum media_type type;

	/* The file base name */
	char* filebasename;

	/* The filename */
	char* filepath;
	
	/* the oid of the directory the
		file is in */
	int64_t directory_oid;

	/* the oid of the media
		information */
	int64_t media_oid;

	/* The directory this file is in */
	struct dir_info* directory;
	
	/* The data of the media */
	void* media_data;
};

struct file_info_query {
	int64_t oid;
	int type;
	char* filebasename;
	char* filepath;
	int64_t directory_oid;
	int64_t media_oid;
};

struct genlist {
	void* data;
	struct genlist* next;
};

struct file_info_list {
	struct file_info* data;
	struct file_info_list* next;
};

/* list for storing dir infos */
struct dir_info_list {
	struct dir_info* data;
	struct dir_info_list* next;
};

/* list for storing dir infos */
struct mp3_info_list {
	struct mp3_info* data;
	struct mp3_info_list* next;
};

/* Context used to load data */
struct load_context {
	char* directory;
	sqlite3* db;
};

void init_file_info_query( struct file_info_query* query );

void init_dir_info_query( struct dir_info_query* query );

void init_mp3_info_query( struct mp3_info_query* query );

/* deletes the nodes of a list and calls `callback` with the
	data of the recently deleted node. If callback is null, then
	nothing happens. */
void disintegrate_list( struct genlist* lst, void(*callback)(void*data) );

void file_info_delete( struct file_info* inf );

void mp3_info_delete( struct mp3_info* inf );

void dir_info_delete( struct dir_info* inf );

int load_directory_root( struct load_context* context );

/* Convert a blob to a hex string that sqlite can
	read */
char* blob_to_sqlite_hex( const void* blob, size_t bloblen );

/* Copy a string into buf and return a pointer to the new
	position */
char* mystrcpy( char* buf, const char* thing, size_t* len );

char* dir_info_to_string( const struct dir_info* inf, char* buf, size_t len );

char* mp3_info_to_string( const struct mp3_info* inf, char* buf, size_t len );

char* file_info_to_string( const struct file_info* inf, char* buf, size_t len );

char* escape_string( const char* str );

/* Returns a list of all dir infos which match
	one of the queries in the query array */
struct dir_info_list* get_dir_info_from_db( sqlite3* db, struct dir_info_query** query, 
												size_t nqueries, size_t limit );

struct dir_info_list* get_dir_info_from_db1( sqlite3* db, struct dir_info_query* query, size_t limit );
/* Setup the database and to add the tables */
void setup_database( sqlite3* handle );

/* Inserts an mp3 entry into the data database referenced
	by handle and then returns the oid of the newly inserted value */
int64_t insert_mp3_info( sqlite3* handle, struct mp3_info* inf );

struct mp3_info_list* get_mp3_info_from_db( sqlite3* db, struct mp3_info_query**,
												size_t nqueries, size_t limit);

struct mp3_info_list* get_mp3_info_from_db1( sqlite3* db, struct mp3_info_query* query, size_t limit );
/* Inserts a directory into the database referenced by handle
	and then returns the oid of the newly inserted dir info */
int64_t insert_dir_info( sqlite3* handle, struct dir_info* inf );

/* Inserts a file into the database referenced by handle
	and then returns the oid of the newly inserted file info */
int64_t insert_file_info( sqlite3* handle, struct file_info* inf );

struct file_info_list* get_file_info_from_db( sqlite3* db, struct file_info_query**,
												size_t nqueries, size_t limit);

struct file_info_list* get_file_info_from_db1( sqlite3* db, struct file_info_query*, size_t limit );

/* Inserts a directory info recursively. Inserting the parents
	if they do not exist */
int64_t insert_dir_info_rec( sqlite3* handle, struct dir_info* inf );

/* Inserts a file info recursively, recursively
	inserting the directory if it does not exist. */
int64_t insert_file_info_rec( sqlite3* handle, struct file_info* inf );

#endif
