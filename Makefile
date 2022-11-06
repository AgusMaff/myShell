CFLAGS = -Wall -pedantic -Werror

all: myShell

myShell: main.o jobsControl.o
	gcc $(CFLAGS) -o myShell main.o jobsControl.o

jobsControl.o: jobsControl.c jobsControl.h
	gcc -c jobsControl.c

main.o: main.c
	gcc -c main.c

.PHONY: clean
clean: 
	rm -f myShell *.o

