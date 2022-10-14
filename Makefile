CC = gcc
CFLAGS = -Werror -Wall -Wextra

default: sshell

sshell.o: sshell.c
	$(CC) $(CFLAGS) -c sshell.c -o sshell.o

sshell: sshell.o
	$(CC) sshell.o -o sshell

clean:
	-rm -f *.o
	-rm sshell