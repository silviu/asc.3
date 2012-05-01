#include "imglib.h"
#include "comm.h"
#include <time.h>
#include <libmisc.h>
#include <stdint.h>

void send_patch_info(int *patch_w, int *patch_h, int *patch_no, int *total_nr_patches, int **spu_patch_id_vector, pixel_t **patches_to_send, int *rand_seed, int *overlap_spu, int ***min_borders)
{
	int i;
	for (i = 0; i < SPU_THREADS; i++) {
		spe_in_mbox_write(ctx[i], (void *)(patch_w), 1, SPE_MBOX_ANY_NONBLOCKING);
		spe_in_mbox_write(ctx[i], (void *)(patch_h), 1, SPE_MBOX_ANY_NONBLOCKING);
		spe_in_mbox_write(ctx[i], (void *)(patch_no), 1, SPE_MBOX_ANY_NONBLOCKING);

		unsigned int out = (unsigned int)spu_patch_id_vector[i];
		spe_in_mbox_write(ctx[i], (void *)(&out), 1, SPE_MBOX_ANY_NONBLOCKING);

		unsigned int send = (unsigned int)patches_to_send;
		spe_in_mbox_write(ctx[i], (void *)(&send), 1, SPE_MBOX_ANY_NONBLOCKING);

		spe_in_mbox_write(ctx[i], (void *)(total_nr_patches), 1, SPE_MBOX_ANY_NONBLOCKING);
		spe_in_mbox_write(ctx[i], (void *)(&rand_seed[i]), 1, SPE_MBOX_ANY_NONBLOCKING);
		spe_in_mbox_write(ctx[i], (void *)(overlap_spu), 1, SPE_MBOX_ANY_NONBLOCKING);

		int j;
		//printf("PPU: SENDING %d\n", *patch_no-1);
		for (j = 0; j < (*patch_no)-1; j++) {
			unsigned int min_borders_address = (unsigned int) min_borders[i][j];
			spe_in_mbox_write(ctx[i], (void *)(&min_borders_address), 1, SPE_MBOX_ANY_NONBLOCKING);
			//printf("PPU: min_borders_address=%p\n", min_borders_address);
		}
	}
}


void get_random_patch_indexes(image img, int patch_w, int patch_h, int *ret_x, int *ret_y)
{
    int max_rand_x = img->width  - patch_w;
    int max_rand_y = img->height - patch_h;

    int x_index = rand() % max_rand_x;
    int y_index = rand() % max_rand_y;

    *ret_x = x_index;
    *ret_y = y_index;
}

struct pixel_t **make_patches(image img, int patch_w, int patch_h, int nr_patches)
{
	int i, j, k;
	pixel_t **patches_to_send = malloc_align(nr_patches * sizeof(pixel_t*), 4);
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

		for (j = 0; j < patch_h; j++) {
			for (k = 0; k < patch_w; k++) {
				int pos_patch = j * patch_w + k;
				int pos_img = x_index + j * img->width + y_index + k;
				patches_to_send[i][pos_patch] = img->buf[pos_img];
			}
		}
	}
	return patches_to_send;
}

void make_final_image(image out_image, int patch_w, int patch_h, int **spu_patch_id_vector, int patch_no, pixel_t **patches_to_send)
{
	int i, j, k;
	for (i = 0; i < SPU_THREADS; i++) {
		for (j = 0; j < patch_no; j++) {
			// index to the patch i want to copy
			int spu_patch_index = spu_patch_id_vector[i][j];

			// offset to patch i want to copy the above patch into
			int offset_to_patch_in_out =  i * patch_w + j * patch_h * out_image->width;
			for (k = 0; k < patch_h; k++) {
				// offset to the line I want to copy
				int spu_patch_offset = k * patch_w;

				// offset to the line in the output image
				int offset_in_out_patch = k * out_image->width;

				// total offset in the output image
				int out_image_offset = offset_to_patch_in_out + offset_in_out_patch;

				// the pixel I want to copy
				pixel_t *spu_patch = patches_to_send[spu_patch_index];

				memcpy(((pixel_t*)out_image->buf) + out_image_offset, ((pixel_t*)spu_patch) + spu_patch_offset, patch_w * sizeof(pixel_t));
			}
		}
	}
}


int **alloc_patch_id_vector(int patch_no)
{
	int i;
	int **spu_patch_id_vector = (int**)malloc_align(SPU_THREADS * sizeof(int*), 4);
	if (spu_patch_id_vector == NULL) {
		perror("PPU: malloc_align failed in main");
		return NULL;
	}
	for (i = 0; i < SPU_THREADS; i++) {
		spu_patch_id_vector[i] = (int*) malloc_align(patch_no * sizeof(int), 4);
		if (spu_patch_id_vector[i] == NULL) {
			perror("PPU: malloc_align failed in main");
			return NULL;
		}
	}
	return spu_patch_id_vector;
}

void free_patch_id_vector(int **spu_patch_id_vector)
{
	int i;
	for (i = 0; i < SPU_THREADS; i++)
		free_align(spu_patch_id_vector[i]);
	free_align(spu_patch_id_vector);
}

int **alloc_aligned_matrix(int h, int w)
{
	int i;
	int **a = malloc_align(h * sizeof(int*), 4);
	if (a == NULL) {
		perror("PPU: malloc_align failed in alloc_aligned_matrix");
		return NULL;
	}
	for (i = 0; i < h; i++) {
		a[i] = malloc_align(w * sizeof(int), 4);
		if (a[i] == NULL) {
			perror("malloc_align failed in alloc_aligned_matrix");
			return NULL;
		}
	}
	return a;
}

void free_aligned_matrix(int **a, int h)
{
	int i;
	for (i = 0; i < h; i++)
		free_align(a[i]);
	free_align(a);
}


int* make_seed_vector()
{
	int i;
	int* rand_seed = malloc_align(SPU_THREADS * sizeof(int), 4);
	if (rand_seed == NULL) {
		perror("malloc_align failed in make_seed_vector()");
		return NULL;
	}

	for (i = 0; i < SPU_THREADS; i++)
		rand_seed[i]=rand()%12345612;

	return rand_seed;
}

void free_seed_vector(int* rand_seed)
{
	free_align(rand_seed);
}

int main(int argc, char **argv)
{
	init_spus();
	srand((unsigned)time(NULL));
	char *fis_in, *fis_out;
	int zoom, rows, cols, i, j,
	overlap_spu, overlap_ppu,
	patch_w, patch_h, nr_patches;	

	if (argc < 8) {
		fprintf(stderr, "Error: Missing some parameters.\n");
		fprintf(stderr, "Run: ./program fis_in fis_out zoom nr_bucati_dim1 nr_bucati_dim2 banda_de_suprapunere_dim1 banda_de_suprapunere_dim2\n");
		return -1;
	}

	fis_in  = argv[1];
	fis_out = argv[2];
	zoom    = atoi(argv[3]);
	rows = atoi(argv[4]);
	cols = atoi(argv[5]);
	overlap_spu = atoi(argv[6]);
	overlap_ppu = atoi(argv[7]);

	
	image img_src = read_ppm(fis_in);
	if (img_src == NULL) {
		fprintf(stderr, "Error reading image file.\n");
		return -1;
	}

	patch_w = (zoom * img_src->width)  / cols;
	patch_h = (zoom * img_src->height) / rows;
	nr_patches = rows * cols;
	printf("PPU: NR PATCHES NECESARY = %d\n", nr_patches);

	int **spu_patch_id_vector = alloc_patch_id_vector(rows);
	if (spu_patch_id_vector == NULL)
		return -1;

	printf("PPU: ZOOM=%d ROWS=%d COLS=%d img->width=%d img->height=%d patch_w=%d patch_h=%d\n", zoom, rows, cols, img_src->width, img_src->height, patch_w, patch_h);

	int* rand_seed = make_seed_vector();
	if (rand_seed == NULL)
		return -1;
	int ***min_borders = malloc_align(SPU_THREADS * sizeof(int**), 4);
	if (min_borders == NULL) {
		perror("PPU: malloc_align failed in main");
		return -1;
	}
	for (i = 0; i < SPU_THREADS; i++) {
		min_borders[i] = alloc_aligned_matrix((rows-1), overlap_spu);
		if (min_borders[i] == NULL)
			return -1;
	}

	pixel_t **patches_to_send = make_patches(img_src, patch_w, patch_h, nr_patches);

	send_patch_info(&patch_w, &patch_h, &rows, &nr_patches, spu_patch_id_vector, patches_to_send, rand_seed, &overlap_spu, min_borders);

	stop_spus();

	int out_img_width = zoom * img_src->width;
	int out_img_height = zoom * img_src->height;
	image img_dst = alloc_img(out_img_width, out_img_height);

	for (i = 0; i < SPU_THREADS; i++) {
		printf("PPU: spu[%d]: ID= ", i);
		for (j = 0; j < rows; j++)
			printf("%d ", spu_patch_id_vector[i][j]);
		printf("\n");
	}

	make_final_image(img_dst, patch_w, patch_h, spu_patch_id_vector, rows, patches_to_send);
	write_ppm(fis_out, img_dst);

	free_img(img_src);
	free_img(img_dst);
	free_seed_vector(rand_seed);
	free_patch_id_vector(spu_patch_id_vector);
	for (i = 0; i < SPU_THREADS; i++)
		free_aligned_matrix(min_borders[i], rows-1);
	return 0;
}
