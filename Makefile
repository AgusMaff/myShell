myShell: main.o
	gcc -o myShell main.o

clean: 
	rm -f myShell *.o

