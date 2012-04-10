build: imglib.c imglib.h main.c
	gcc -Wall -g main.c imglib.c imglib.h -o main
run:
	./main input/caramida.ppm output/caramida.ppm 2 3 8 10 10

gdb:
	gdb --args ./main input/caramida.ppm output/caramida.ppm 2 3 8 10 10

clean:
	rm -rf *.o main