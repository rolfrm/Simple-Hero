OPT = -g
SOURCES = simplehero.c ../bitguy/linmath.c ../bitguy/utils.c renderer.c coroutines.c
CC = gcc
TARGET = run
OBJECTS =$(SOURCES:.c=.o)
LDFLAGS=-L. -L../libconcurrency-read-only/  $(OPT) -Wextra #setrlimit on linux 
LIBS= -lSDL2 -lSDL2_ttf -lSDL2_image -lSDL2_gfx -lGL -lm -lpthread
CFLAGS =  -I../libconcurrency-read-only/libconcurrency/ -std=c11 -c $(OPT) -Wall -Wextra -Werror=implicit-function-declaration -Wformat=0  -fno-strict-aliasing
all: $(TARGET)
$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $@

.c.o: $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@ -MMD -MF $@.depends
depend: h-depend
clean:
	rm $(OBJECTS) $(TARGET) *.o.depends
-include $(OBJECTS:.o=.o.depends)

