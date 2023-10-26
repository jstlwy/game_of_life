OS := $(shell uname)
WFLAGS := -Wall -Wextra -Werror -Wpedantic
ifeq ($(OS), Darwin)
CC := clang
CFLAGS := $(WFLAGS) -F/Library/Frameworks -O
LDFLAGS := -F/Library/Frameworks/ -framework SDL2 -rpath /Library/Frameworks
else
CC := gcc
CFLAGS := $(WFLAGS) -march=native -O
LDFLAGS := -lm -lSDL2
endif

src := $(wildcard *.c)
hdr := $(wildcard *.h)
obj := $(src:%.c=%.o)
bin := game_of_life

.PHONY: all clean

all: $(bin)

$(bin): $(obj)
	$(CC) $^ -o $@ $(LDFLAGS)

main.o: main.c $(hdr)
graphics.o: graphics.c
timing.o: timing.c
$(obj):
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(obj) $(bin)

