linux:
	gcc -o todo todo.c `pkg-config --cflags --libs sdl2` `sdl2-config --cflags --libs` -lSDL2_ttf -lSDL2_image

leif:
	gcc -o todo todo.c -lleif -lglfw -lm -lGL -lclipboard 

windows:
	x86_64-w64-mingw32-gcc -o todo.exe todo.c `/usr/x86_64-w64-mingw32/usr/bin/sdl2-config --libs` -lSDL2_ttf -lSDL2_image

