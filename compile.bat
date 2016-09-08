gcc -c -Ofast -std=c99 -o main.o main.c
gcc -o main.exe main.o -lglu32 glut32.lib -lopengl32
pause
rem gcc -o main.exe main.o -lglu32 -mwindows glut32.lib -lopengl32 -Wl,--subsystem,windows