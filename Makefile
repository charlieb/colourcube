CC=gcc
CFLAGS=--std=c99 -pedantic -Wall
SOURCES=$(wildcard *.c)
OBJECTS=$(SOURCES:.c=.o)
DEST=.
EXE=colourcube
INCLUDES=`pkg-config --cflags libpng glib-2.0`
LIBS=`pkg-config --libs libpng glib-2.0`

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

exe: $(OBJECTS)
	$(CC) $(CFLAGS) $(LIBS) $(OBJECTS) -o $(DEST)/$(EXE)

debug: CFLAGS += -g
debug: exe

profile: CFLAGS += -g -pg -O3
profile: exe

release: CFLAGS += -O3
release: exe

clean:
	@ - rm $(DEST)/$(EXE) $(OBJECTS)
