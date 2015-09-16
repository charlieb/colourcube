CC=gcc
CFLAGS=-ansi -pedantic -Wall
SOURCES=$(wildcard *.c)
OBJECTS=$(SOURCES:.c=.o)
DEST=.
EXE=colourcube
INCLUDES=
LIBS=

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

exe: $(OBJECTS)
	$(CC) $(CFLAGS) $(LIBS) $(OBJECTS) -o $(DEST)/$(EXE)

debug: CFLAGS += -g
debug: exe

release: CFLAGS += -O2
release: exe

clean:
	@ - rm $(DEST)/$(EXE) $(OBJECTS)
