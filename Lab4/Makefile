TARGET = server client

CFLAGS = -lpthread

all: ${TARGET}
server: server.c
	gcc server.c -o server ${CFLAGS}
client: client.c
	gcc client.c -o client ${CFLAGS}
clean:  
	rm -f ${TARGET}