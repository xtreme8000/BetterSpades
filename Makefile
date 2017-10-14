BIG_INCLUDE = *.c

main: ${BIG_INCLUDE}
	gcc -std=c99 -o main main.c -lc -lm -lglfw -lGLU -lenet -lGL ./libdeflate/libdeflate.a -g
