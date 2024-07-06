
required library installations;  
> sudo apt install libsdl2-dev;  
> sudo apt install libsdl2-image-dev;  
  
compile && run:  
> gcc main.c -o particle-sim \`sdl2-config --cflags --libs\` -lSDL2_image -lm; ./particle-sim
