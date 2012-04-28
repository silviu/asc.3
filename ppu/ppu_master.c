#include "imglib.h"
#include "comm.h"
#include <time.h>
#include <libmisc.h>
#include <stdint.h>

void send_patch_info(int *patch_w, int *patch_h, int *patch_no, image output)
{
	int i;
	for (i = 0; i < SPU_THREADS; i++) {
		spe_in_mbox_write(ctx[i], (void *)(patch_w), 1, SPE_MBOX_ANY_NONBLOCKING);
		spe_in_mbox_write(ctx[i], (void *)(patch_h), 1, SPE_MBOX_ANY_NONBLOCKING);
		spe_in_mbox_write(ctx[i], (void *)(patch_no), 1, SPE_MBOX_ANY_NONBLOCKING);
		unsigned int out = (unsigned int)&GET_PIXEL(output, 0, (i * *patch_w));
		spe_in_mbox_write(ctx[i], (void *)(&out), 1, SPE_MBOX_ANY_NONBLOCKING);
	}
}

struct pixel_t **make_patches(image img, int patch_w, int patch_h, int nr_patches)
{
	int i, j, k;
	struct pixel_t **patches_to_send = malloc_align(nr_patches * sizeof(struct pixel_t*), 4);
	if (patches_to_send == NULL) {
		perror("malloc_align failed in make_patches");
		return NULL;
	}
	for (i = 0; i < nr_patches; i++) {
		patches_to_send[i] = malloc_align(patch_w * patch_h * sizeof(pixel_t), 4);
		if (patches_to_send[i] == NULL) {
			perror("malloc_align failed in make_patches");
			free_align(patches_to_send);
			return NULL;
		}
		int x_index, y_index;
		get_random_patch_indexes(img, patch_w, patch_h, &x_index, &y_index);
		printf("X_INDEX=%d Y_INDEX=%d\n", x_index, y_index);
		printf("GET_PIXEL=%p\n", GET_PIXEL(img, x_index, y_index));

		for (j = 0; j < patch_w; j++) {
			for (k = 0; k < patch_h; k++) {
				int pos_img = (x_index + j) * img->width + y_index + k;
				int pos_patch = j * patch_w + k;
				//printf("Copy pixel from img[%d] to patch[%d]\n",
				//	pos_img, pos_patch);
				patches_to_send[i][pos_patch] = img->buf[pos_img];
			}
		}


	}
	return patches_to_send;
}

void send_first_patch(image img, int patch_w, int patch_h)
{
	int i;
	for (i = 0; i < SPU_THREADS; i++) {
		int x_index, y_index;
		get_random_patch_indexes(img, patch_w, patch_h, &x_index, &y_index);
		unsigned int send = (unsigned int)&GET_PIXEL(img, x_index, y_index);
		//printf("TO SEND: %p\n", send);
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
	patch_w, patch_h, nr_patches;
	

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
	nr_patches = nr_bucati_dim1 * nr_bucati_dim2;
	printf("NR+PATCHES NECESARY = %d\n", nr_patches);

	image final_image = alloc_img(img->width * zoom, img->height * zoom);

	send_patch_info(&patch_w, &patch_h, &nr_bucati_dim1, final_image);
	pixel_t **patches_to_send = make_patches(img, patch_w, patch_h, nr_patches);
	int i;
	for (i = 0; i < nr_patches; i++)
		printf("ZZZZZZZZZZ: %p\n", patches_to_send[i]);

	//send_first_patch(img, patch_w, patch_h);

    write_ppm(fis_out, img);

	free_img(img);
	stop_spus();
	return 0;
}
