all: imglib.c imglib.h main.c
	gcc -Wall -g main.c imglib.c imglib.h -o main