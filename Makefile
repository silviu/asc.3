DIRS=ppu spu
include ${CELL_TOP}/buildutils/make.footer


run:
	ppu/ppu_master ppu/input/caramida.ppm ppu/output/caramida.ppm 3 4 8 10 10


gdb:
	gdb --args ppu/ppu_master ppu/input/caramida.ppm ppu/output/caramida.ppm 3 4 8 10 10

clean:
	rm -rf *.o ppu/ppu_master