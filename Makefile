target=nm_fremd
OBJS=$(target).o auth_protocol.o read_config.o parse_cmd.o debug.o heart_beat.o database_query.o\
			auth_request.o  access_info.o 
CFLAGS=-g -I/usr/local/include/apr-1  -D_LARGEFILE64_SOURCE -Wall
LIBS=-lpthread /usr/local/lib/libaprutil-1.so  /usr/local/lib/libapr-1.so   -Wl,-rpath=/usr/local/lib
CC=gcc
all:$(target) auth_client
$(target):$(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $(target)
auth_client:auth_protocol.o auth_client.o
	$(CC) $(CFLAGS) auth_protocol.o auth_client.o $(LIBS) -o $@
%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@
install:
	cp $(target) /usr/local/bin
	cp $(target).conf /etc/
clean:
	rm -rf $(OBJS) $(target) authClient authClient.o *.o
