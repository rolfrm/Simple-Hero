OPT = -g
SOURCES =  simplehero.c ../iron/linmath.c ../iron/utils.c renderer.c coroutines.c simplehero-ai.c circle.c sdl_utils.c uivector.c event.c sdl_event.c lisp_parser.c ld32_game.c lisp_interpreter.c game_controller.c game_state.c vox.c vox_internal.c vox_raster.c lisp_compiler.c lisp_types.c lisp_std_types.c ../iron/mem.c ../iron/fileio.c ../iron/array.c ../iron/math.c ../iron/time.c ../iron/hashtable.c ../iron/log.c
CC = gcc
TARGET = run
OBJECTS =$(SOURCES:.c=.o)
LDFLAGS=-ldl -L. -L../libconcurrency-read-only/  $(OPT) -Wextra -flto  #setrlimit on linux 
LIBS= -ldl -lSDL2 -lSDL2_ttf -lSDL2_image -lSDL2_gfx -lGL -lm -lpthread 

CFLAGS = -Itcc -I.. -I../libconcurrency-read-only/libconcurrency/ -std=c11 -c $(OPT) -Wall -Wextra -Werror=implicit-function-declaration -Wformat=0  -g3 -O0 -D_GNU_SOURCE -fdiagnostics-color

all: $(TARGET)
$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) tcc/libtcc.a -ldl -o $@

.c.o: $(HEADERS)
	$(CC) $(CFLAGS) $< -o $@ -MMD -MF $@.depends
depend: h-depend
clean:
	rm $(OBJECTS) $(TARGET) *.o.depends
-include $(OBJECTS:.o=.o.depends)

