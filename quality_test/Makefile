CC=gcc
CFLAGS=-c -g -std=c99 `pkg-config --cflags jack` -I ../qoa/
LDFLAGS=`pkg-config --libs jack`
SOURCES=main.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=qoa_quality_test

all: $(EXECUTABLE)

run: all
	./$(EXECUTABLE)

clean:
	rm -f ${EXECUTABLE} ${OBJECTS}

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

.c.o:
	$(CC) $< -o $@ $(CFLAGS)
