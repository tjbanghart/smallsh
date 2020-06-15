CC = gcc

smallsh: 
	$(CC) -o smallsh smallsh.c

clean:
	rm -f smallsh results
	