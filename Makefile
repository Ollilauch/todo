linux:
	gcc -o todo todo.c `pkg-config --cflags --libs sdl2` `sdl2-config --cflags --libs` -lSDL2_ttf -lSDL2_image

debug:
	gcc -mno-avx -Wall -Werror -o todo todo.c `pkg-config --cflags --libs sdl2` `sdl2-config --cflags --libs` -lSDL2_ttf -lSDL2_image

leif:
	gcc -o todo todo.c -lleif -lglfw -lm -lGL -lclipboard 

windows:
	i686-w64-mingw32-gcc -o windows-binary/todo.exe todo.c `/usr/i686-w64-mingw32/mingw/bin/sdl2-config --cflags --libs` -lSDL2 -lSDL2_ttf -lSDL2_image
