all: libmfs.so server

libmfs.so:
	gcc -shared -fPIC -ggdb -o libmfs.so mfs.c cNetworkLib.c udp.c

client: libmfs.so
	gcc -ggdb -L. -lmfs -o client client.c

server:
	gcc -o server server.c sNetworkLib.c lfs.c udp.c

clean:
	rm -rf libmfs.so client server