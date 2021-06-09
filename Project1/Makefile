main: main.o strbuf.o
	gcc -g -Wvla -Wall -fsanitize=address,undefined -o main main.o strbuf.o

main.o: main.c strbuf.h
	gcc -c -g -Wvla -Wall -fsanitize=address,undefined main.c

strbuf.o: strbuf.c strbuf.h
	gcc -c -g -Wvla -Wall -fsanitize=address,undefined strbuf.c

clean:
	rm -f *.o main