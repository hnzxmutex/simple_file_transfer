objs = socket_wrapper.o file_transfer.o md5.o
server_objects = $(objs) server.o
client_objects = $(objs) client.o

default: file_server file_client
file_server: $(server_objects)
	gcc $(server_objects) -o file_server
file_client: $(client_objects)
	gcc $(client_objects) -o file_client
server.o:server.c file_transfer.h socket_wrapper.h md5.h
	gcc -c server.c
client.o: client.c file_transfer.h socket_wrapper.h md5.h
	gcc -c client.c
socket_wrapper.o: socket_wrapper.c socket_wrapper.h
	gcc -c socket_wrapper.c
file_transfer.o: file_transfer.c file_transfer.h md5.h
	gcc -c file_transfer.c
md5.o: md5.c md5.h
	gcc -c md5.c
.PHONY:clean
clean:
	-rm -f *.o file_server file_client ./save_path/*
	-rmdir ./save_path
