#include "imglib.h"
#include "comm.h"
#include <time.h>
#include <libmisc.h>

void send_patch_info(int *patch_w, int *patch_h, int *patch_no, image output)
{
	int i;
	for (i = 0; i < SPU_THREADS; i++) {
		spe_in_mbox_write(ctx[i], (void *)(patch_w), 1, SPE_MBOX_ANY_NONBLOCKING);
		spe_in_mbox_write(ctx[i], (void *)(patch_h), 1, SPE_MBOX_ANY_NONBLOCKING);
		spe_in_mbox_write(ctx[i], (void *)(patch_no), 1, SPE_MBOX_ANY_NONBLOCKING);
		int out = &GET_PIXEL(output, 0, (i * *patch_w));
		spe_in_mbox_write(ctx[i], (void *)(&out), 1, SPE_MBOX_ANY_NONBLOCKING);
	}

}

void send_first_patch(image img, int patch_w, int patch_h)
{
	int i;
	for (i = 0; i < SPU_THREADS; i++) {
		int x_index, y_index;
		get_random_patch_indexes(img, patch_w, patch_h, &x_index, &y_index);
		printf("XXXXXXXXXXX: %d %d\n", x_index, y_index);
		int send = &GET_PIXEL(img, x_index, y_index);
		printf("TO SEND: %p\n", send);
		spe_in_mbox_write(ctx[i], (void *)(&send), 1, SPE_MBOX_ANY_NONBLOCKING);
	}
}

int main(int argc, char **argv)
{
	init_spus();
	srand((unsigned)time(NULL));
	char *fis_in, *fis_out;
	int zoom, nr_bucati_dim1, nr_bucati_dim2, 
	banda_de_suprapunere_dim1, banda_de_suprapunere_dim2,
	patch_w, patch_h;
	

	if (argc < 8) {
		fprintf(stderr, "Error: Missing some parameters.\n");
		fprintf(stderr, "Run: ./program fis_in fis_out zoom nr_bucati_dim1 nr_bucati_dim2 banda_de_suprapunere_dim1 banda_de_suprapunere_dim2\n");
		return -1;
	}

	fis_in  = argv[1];
	fis_out = argv[2];
	zoom    = atoi(argv[3]);
	nr_bucati_dim1 = atoi(argv[4]);
	nr_bucati_dim2 = atoi(argv[5]);
	banda_de_suprapunere_dim1 = atoi(argv[6]);
	banda_de_suprapunere_dim2 = atoi(argv[7]);

	image img = read_ppm(fis_in);
	if (img == NULL) {
		fprintf(stderr, "Error reading image file.\n");
		return -1;
	}
	

	patch_w = (zoom * img->width)/nr_bucati_dim1;
	patch_h = (zoom * img->height)/nr_bucati_dim2;

	image final_image = alloc_img(img->width * zoom, img->height * zoom);

	send_patch_info(&patch_w, &patch_h, &nr_bucati_dim1, final_image);

	send_first_patch(img, patch_w, patch_h);

    //write_ppm(fis_out, final_image);

	free_img(img);
	stop_spus();
	return 0;
}
