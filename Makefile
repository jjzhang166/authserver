target=nm_fremd
OBJS=$(target).o authProtocol.o readConfig.o parseCmd.o debug.o
CFLAGS=-g -I/usr/local/include/apr-1  -D_LARGEFILE64_SOURCE -Wall
LIBS=-lpthread /usr/local/lib/libaprutil-1.so  /usr/local/lib/libapr-1.so   -Wl,-rpath=/usr/local/lib
CC=gcc
all:$(target) authClient
$(target):$(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $(target)
authClient:authProtocol.o authClient.o
	$(CC) $(CFLAGS) authProtocol.o authClient.o $(LIBS) -o authClient
%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@
ins:
	cp $(target) /usr/local/bin
	cp $(target).conf /etc/
clean:
	rm -rf $(OBJS) $(target) authClient authClient.o
