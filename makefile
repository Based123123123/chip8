CFLAGS=-Wall -Wextra -Werror -Wno-pointer-arith
all:
	g++ chip8.cpp display.cpp main.cpp -o main $(CFLAGS) `pkg-config sdl3 --cflags --libs`
