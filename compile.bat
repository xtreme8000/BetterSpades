gcc -c -Ofast -std=c99 -o main.o main.c
gcc -o main.exe main.o -lglu32 -lfreeglut -lopengl32 -L. -Wl,-Bstatic -lenet -lws2_32 -lwinmm -ldeflate
pause
run_min_gfx
rem -Wl,--subsystem,windows
