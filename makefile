C = gcc
CFLAGS = -ggdb -Wall -std=c99
PROGRAM = my_shell
OBJECTS = ${PROGRAM:=.o}

all: my_shell

my_shell: $(OBJECTS)
	$(C) $(CFLAGS) $? -o $@

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJECTS)
	
strip:
	strip -g $(PROGRAM)
