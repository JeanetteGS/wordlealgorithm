wordle.o: wordle.c
	gcc -g -Wall -Werror -o wordle.out wordle.c

planBWordle.o: planBWordle.c
	gcc -g -Wall -Werror -o planBWordle.out planBWordle.c

clean:
	rm -fr *.o *.out