CC=gcc

# Dev build
CFLAGS=-c -std=gnu99 -Wfatal-errors -Wall -g -O0 -I../allegro/include/ -Iinclude/ -DDEBUG
LDFLAGS=-lalleg

SOURCES=$(wildcard src/*.c)
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=imgui.exe

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	@del src\*.o

.PHONY: rebuild
rebuild: clean all

run: all
	imgui.exe
	if errorlevel 0 cls
