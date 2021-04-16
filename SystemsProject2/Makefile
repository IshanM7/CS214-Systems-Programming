main: main.o strbuf.o LL.o Queue.o
	gcc -g -pthread -Wvla -Wall -fsanitize=address,undefined -o main main.o strbuf.o LL.o Queue.o -lm

main.o: main.c strbuf.h
	gcc -c -g -Wvla -Wall -fsanitize=address,undefined main.c

strbuf.o: strbuf.c strbuf.h
	gcc -c -g -Wvla -Wall -fsanitize=address,undefined strbuf.c

LL.o: LL.c LL.h
	gcc -c -g -Wvla -Wall -fsanitize=address,undefined LL.c

Queue.o: Queue.c Queue.h
	gcc -c -g -Wvla -Wall -fsanitize=address,undefined Queue.c

clean:
	rm -f *.o main