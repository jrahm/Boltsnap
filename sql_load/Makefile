MEDIA_LOAD_OBJS=mediaload.o mediaload_dirinfo.o mediaload_fileinfo.o \
					mediaload_mp3info.o mediaload_loader.o
MEDIA_LOAD_BIN_OBS=bsupdatedb.o querydb.o



AR=ar

LOADER_BINARY=$(OUTDIR)/bin/boltupdatedb

QUERYDB_BINARY=$(OUTDIR)/bin/querydb

ARCHIVE=libmediaload.a

all: | init mediaload_all 

mediaload.o: mediaload.c mediaload.h
	$(CC) $(CFLAGS) $(INCLUDE) -c mediaload.c

mediaload_dirinfo.o: mediaload_dirinfo.c mediaload.h
	$(CC) $(CFLAGS) $(INCLUDE) -c mediaload_dirinfo.c

mediaload_fileinfo.o: mediaload_fileinfo.c mediaload.h
	$(CC) $(CFLAGS) $(INCLUDE) -c mediaload_fileinfo.c

mediaload_mp3info.o: mediaload_mp3info.c mediaload.h
	$(CC) $(CFLAGS) $(INCLUDE) -c mediaload_mp3info.c

mediaload_loader.o: mediaload_loader.c mediaload.h
	$(CC) $(CFLAGS) $(INCLUDE) -c mediaload_loader.c

querydb.o: querydb.c
	$(CC) $(CFLAGS) $(INCLUDE) -c querydb.c

bsupdatedb.o: bsupdatedb.c
	$(CC) $(CFLAGS) $(INCLUDE) -c bsupdatedb.c

clean:
	- rm -rf $(ARCHS)
	- rm -rf *.o $(ARCHIVE)
	- rm -rf *.sqlite3
	- rm -rf .*.sw*

init:
	- mkdir -p $(OUTDIR)/bin
	- mv $(OUTDIR)/*.o . 


mediaload_all: $(MEDIA_LOAD_OBJS) $(MEDIA_LOAD_BIN_OBS)
	$(AR) -r $(ARCHIVE) $(MEDIA_LOAD_OBJS)
	$(LD) -o $(LOADER_BINARY) bsupdatedb.o -L. -lmediaload -lsqlite3 -lavformat -lavutil
	$(LD) -o $(QUERYDB_BINARY) querydb.o -L. -lmediaload -lsqlite3 -ljansson
	- mkdir -p $(OUTDIR) 
	- mv *.o *.a $(OUTDIR) 
