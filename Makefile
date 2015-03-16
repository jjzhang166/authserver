target=authServer
OBJS=authServer.o authProtocol.o
CFLAGS=-g -I/usr/local/include/apr-1  -D_LARGEFILE64_SOURCE
LIBS=-lpthread /usr/local/lib/libaprutil-1.so  /usr/local/lib/libapr-1.so   -Wl,-rpath=/usr/local/lib
CC=gcc
all:$(target) authClient
authServer:$(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $(target)
authClient:authProtocol.o authClient.o
	$(CC) $(CFLAGS) authProtocol.o authClient.o $(LIBS) -o authClient
clean:
	rm $(OBJS) $(target)
