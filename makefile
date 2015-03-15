OPT = -g
SOURCES = simplehero.c
CC = gcc
TARGET = run
OBJECTS =$(SOURCES:.c=.o)
LDFLAGS=-L.  $(OPT) -Wextra #setrlimit on linux 
LIBS= -lSDL2 -lSDL2_ttf -lSDL2_image
CFLAGS = -std=c11 -c $(OPT) -Wall -Wextra -Werror=implicit-function-declaration -Wformat=0 -O3
all: $(TARGET)
$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $@

.c.o: $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@ -MMD -MF $@.depends
depend: h-depend
clean:
	rm $(OBJECTS) $(TARGET) *.o.depends
-include $(OBJECTS:.o=.o.depends)

