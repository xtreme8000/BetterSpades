gcc -c -Ofast -std=c99 -o main.o main.c
rem gcc -DSTANDALONE -o main.exe main.o -lglu32 -lfreeglut -lopengl32
objconv -fomf main.o openglmodule.obj
pause
run_max_gfx
rem -Wl,--subsystem,windows