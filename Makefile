SDL:
	gcc -o todo todo.c `pkg-config --cflags --libs sdl2` `sdl2-config --cflags --libs` -lSDL2_ttf -lSDL2_image

leif:
	gcc -o todo todo.c -lleif -lglfw -lm -lGL -lclipboard 

