server: server.o strbuf.o
	gcc -g -pthread -Wvla -Wall -fsanitize=address,undefined -o server server.o strbuf.o

server.o: server.c strbuf.h
	gcc -c -g -Wvla -Wall -fsanitize=address,undefined server.c

strbuf.o: strbuf.c strbuf.h
	gcc -c -g -Wvla -Wall -fsanitize=address,undefined strbuf.c