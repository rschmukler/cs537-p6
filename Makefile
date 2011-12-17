libmfs.so:
	gcc -shared -fPIC -o libmfs.so mfs.c cNetworkLib.c udp.c

client: libmfs.so
	gcc -L. -lmfs -o client client.c

server:
	gcc -o server server.c sNetworkLib.c udp.c