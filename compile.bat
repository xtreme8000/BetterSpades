gcc -c -Ofast -std=c99 -o main.o main.c
gcc -o main.exe main.o -lglu32 -lfreeglut -lopengl32
pause
run_max_gfx
rem -Wl,--subsystem,windows