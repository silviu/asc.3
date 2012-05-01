#include <stdio.h>
#include <spu_mfcio.h>
#include <libmisc.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define MAX_DMA_TRANSFER 16384
#define waitag(t) mfc_write_tag_mask(1<<t); mfc_read_tag_status_all();
#define MIN2(x, y) (((x) < (y)) ? (x) : (y))
#define MIN3(a, b, c) (MIN2(MIN2((a), (b)), (c)))

struct pixel_t {
	char r;
	char g;
	char b;
};
typedef struct pixel_t pixel_t;

void get_patch_info(int *patch_w, int *patch_h, int *patch_no, int *total_nr_patches, int **spu_patch_id_vector, pixel_t*** patch_vector, int *rand_seed, int *overlap_size, int ***ppu_min_borders_adress)
{
	// get patch width
	while (spu_stat_in_mbox()<=0);
	*patch_w = (int)spu_read_in_mbox();

	//get patch height
	while (spu_stat_in_mbox()<=0);
	*patch_h = (int)spu_read_in_mbox();

	//get number of patches needed to get
	while (spu_stat_in_mbox()<=0);
	*patch_no = (int)spu_read_in_mbox();

	//get start of my zone
	while (spu_stat_in_mbox()<=0);
	*spu_patch_id_vector = (int*)spu_read_in_mbox();

	//get patch vector
	while (spu_stat_in_mbox()<=0);
	*patch_vector = (pixel_t**)spu_read_in_mbox();

	//get total number of patches
	while (spu_stat_in_mbox()<=0);
	*total_nr_patches = (int)spu_read_in_mbox();

	//get random seed
	while (spu_stat_in_mbox()<=0);
	*rand_seed = (int)spu_read_in_mbox();

	//get overlap_w
	while (spu_stat_in_mbox()<=0);
	*overlap_size = (int)spu_read_in_mbox();

	int i;
	*ppu_min_borders_adress = malloc_align((*patch_no - 1) * sizeof (int*), 4);
	//printf("GETTING %d\n", *patch_no-1);
	for (i = 0; i < ((*patch_no)-1); i++) {
		while (spu_stat_in_mbox()<=0);
		*ppu_min_borders_adress[i] = (int*)spu_read_in_mbox();
		//printf("ppu_min_borders_adress=%p\n", *ppu_min_borders_adress[i]);
	}

}

pixel_t **dma_get_patch_vector_from_ppu(pixel_t **patch_vector, int total_nr_patches)
{
	uint32_t tag_id = mfc_tag_reserve();
	if (tag_id == MFC_TAG_INVALID) {
		printf("SPU: ERROR can't allocate tag ID\n"); 
		return NULL;
	}

	pixel_t **all_patches = malloc_align(total_nr_patches * sizeof(pixel_t*), 4);

	mfc_get((void*)all_patches, (unsigned int)(void*)patch_vector, (uint32_t)total_nr_patches * sizeof(void *), tag_id, 0, 0);
	waitag(tag_id);

	return all_patches;
}

pixel_t *get_random_patch_address(pixel_t **all_patches, int total_nr_patches, int *id_vector_index)
{
	int pos = rand() % total_nr_patches;
	*id_vector_index = pos;
	return all_patches[pos];
}

void dma_get_patch(pixel_t *patch_address, int patch_w, int patch_h, pixel_t *patch)
{
	int i;
	int patch_size = patch_w * patch_h * sizeof(pixel_t);	
	int max_patch_count = patch_size / MAX_DMA_TRANSFER;


	uint32_t tag_id = mfc_tag_reserve();
	if (tag_id == MFC_TAG_INVALID) {
		printf("SPU: ERROR can't allocate tag ID\n"); 
		return;
	}

	for (i = 0; i < max_patch_count; i++) {
		mfc_get(((char*)patch) + i * MAX_DMA_TRANSFER, (unsigned int)((char*)patch_address) + i * MAX_DMA_TRANSFER, (uint32_t)MAX_DMA_TRANSFER, tag_id, 0, 0);
		waitag(tag_id);
	}
	
	int rest = patch_size%MAX_DMA_TRANSFER;

	mfc_get(((char*)patch) + max_patch_count * MAX_DMA_TRANSFER, (unsigned int)((char*)patch_address) + max_patch_count * MAX_DMA_TRANSFER, (uint32_t)rest, tag_id, 0, 0);	
	waitag(tag_id);
}

void dma_send_patch_id_vector(int* ppu_id_pacth_vector_address, int *spu_patch_id_vector, int patch_no)
{
	uint32_t tag_id = mfc_tag_reserve();
	if (tag_id == MFC_TAG_INVALID) {
		printf("SPU: ERROR can't allocate tag ID\n"); 
		return;
	}
	mfc_put((void*)spu_patch_id_vector, (unsigned int)((void*)ppu_id_pacth_vector_address), (uint32_t)patch_no * sizeof(int), tag_id, 0, 0);	
	waitag(tag_id);
}

int **alloc_matrix(int h, int w)
{
	int i;
	int **e = malloc(h * sizeof(int*));
	if (e == NULL) {
		perror("SPU: malloc failed in calculate_error");
		return NULL;
	}
	for (i = 0; i < h; i++) {
		e[i] = malloc(w * sizeof(int));
		if (e[i] == NULL) {
			perror("malloc failed in calculate_error");
			return NULL;
		}
	}
	return e;
}

void free_matrix(int **e, int h)
{
	int i;
	for (i = 0; i < h; i++)
		free(e[i]);
	free(e);
}

void calculate_error_matrix(int** error_matrix, pixel_t *patch_old, pixel_t *patch_curr, int patch_w, int patch_h, int overlap_size)
{
	int i, j;
	int patch_old_offset = patch_h - overlap_size;
	pixel_t *patch_old_act = patch_old + patch_old_offset;
	pixel_t *patch_curr_act = patch_curr;
	for (i = 0; i < overlap_size; i++) {
		for (j = 0; j < patch_w; j++) {
			int red   = patch_old_act[j].r - patch_curr_act[j].r;
			int green = patch_old_act[j].g - patch_curr_act[j].g;
			int blue  = patch_old_act[j].b - patch_curr_act[j].b;
			error_matrix[i][j] =  red * red + green * green + blue * blue;
			/*if (i == 0 && j == 0) {
				printf("PATCH_OLD.r=%d PATCH_CURR.r=%d\nPATCH_OLD.g=%d PATCH_CURR.g=%d\nPATCH_OLD.b=%d PATCH_CURR.b=%d\n", patch_old_act[j].r, patch_curr_act[j].r, patch_old_act[j].g, patch_curr_act[j].g, patch_old_act[j].b, patch_curr_act[j].b);
				printf("RESULT=%d", error_matrix[i][j]);
			}*/
		}
		patch_old_act  += patch_w;
		patch_curr_act += patch_w;
	}
}

void calculate_min_border_matrix(int **min_border_matrix, int **error_matrix, int overlap_size, int patch_w)
{
	int i, j;
	for (j = 0; j < patch_w; j++)
		min_border_matrix[0][j] = error_matrix[0][j];

	for (i = 1; i < overlap_size; i++)
		for (j = 0; j < patch_w; j++)
			if (j == 0)
				min_border_matrix[i][j] = error_matrix[i][j] + MIN2(min_border_matrix[i-1][j], min_border_matrix[i-1][j+1]);
			else
				min_border_matrix[i][j] = error_matrix[i][j] + MIN3(min_border_matrix[i-1][j-1], min_border_matrix[i-1][j], min_border_matrix[i-1][j+1]);
}

int *get_min_border(int **min_border_matrix, int overlap_size, int patch_w)
{
	int i, j;
	int *min_border = malloc_align(overlap_size * sizeof(int), 4);
	if (min_border == NULL) {
		perror("SPU: malloc_align failed in get_min_border");
		return NULL;
	}
	int start_x = overlap_size - 1;
	int start_y = 0;
	int min = min_border_matrix[start_x][start_y];
	for (i = 1; i < patch_w; i++)
		if (min_border_matrix[start_x][i] < min) {
			min = min_border_matrix[start_x][i];
			start_y = i;
		}
	min_border[start_x] = start_y;
	for (i = start_x - 1; i >= 0; i--) {
		min = min_border_matrix[i][0];
		for (j = 1; j < patch_w; j++)
			if (min_border_matrix[i][j] < min) {
				min = min_border_matrix[i][j];
				min_border[i] = j;
			}
	}
	return min_border;
}


int *min_error_border(int **error_matrix, int **min_border_matrix, pixel_t *patch_old, pixel_t *patch_curr,int patch_w, int patch_h, int overlap_size)
{
	calculate_error_matrix(error_matrix, patch_old, patch_curr, patch_w, patch_h, overlap_size);
	calculate_min_border_matrix(min_border_matrix, error_matrix, overlap_size, patch_w);
	int *min_border = get_min_border(min_border_matrix, overlap_size, patch_w);
	return min_border;
}

void dma_send_min_borders(int **ppu_min_boreders_adress, int **min_borders_for_ppu, int count, int overlap_size)
{
	int i;
	uint32_t tag_id = mfc_tag_reserve();
	if (tag_id == MFC_TAG_INVALID) {
		printf("SPU: ERROR can't allocate tag ID\n"); 
		return;
	}

	for (i = 0; i < count; i++) {
		int size = overlap_size * sizeof(int);
		size += size % 16;
		printf("ppu_min_boreders_adress=%p\n", ppu_min_boreders_adress[i]);
		mfc_put((int*)min_borders_for_ppu[i], (unsigned int)((int*)ppu_min_boreders_adress[i]), (uint32_t)size, tag_id, 0, 0);	
		waitag(tag_id);
	}
}

int main(unsigned long long speid, unsigned long long argp, unsigned long long envp)
{
	unsigned int id = (unsigned int) argp;
	int patch_w, patch_h, patch_no, total_nr_patches, rand_seed, overlap_size, i;
	int *ppu_id_patch_vector_address, *spu_patch_id_vector, id_vector_index_old, id_vector_index_curr;
	pixel_t **patch_vector, **all_patches, *patch_old_address, *patch_old = NULL, *patch_curr_address, *patch_curr = NULL;
	int **ppu_min_borders_adress;

	get_patch_info(&patch_w, &patch_h, &patch_no, &total_nr_patches, &ppu_id_patch_vector_address, &patch_vector, &rand_seed, &overlap_size, &ppu_min_borders_adress);
	
	printf("ppu_min_borders_adress=");
	for ( i = 0; i < patch_no-1; i++)
		printf("%p ", ppu_min_borders_adress[i]);
	printf("\n");
	srand(rand_seed);

	all_patches = dma_get_patch_vector_from_ppu(patch_vector, total_nr_patches);
	spu_patch_id_vector = malloc_align(patch_no * sizeof(int), 4);
	patch_old = malloc_align(patch_w * patch_h * sizeof(pixel_t), 4);
	patch_curr = malloc_align(patch_w * patch_h * sizeof(pixel_t), 4);
	int **error_matrix = alloc_matrix(overlap_size, patch_w);
	int **min_border_matrix = alloc_matrix(overlap_size, patch_w);
	if (error_matrix == NULL)
		return -1;

	patch_old_address = get_random_patch_address(all_patches, total_nr_patches, &id_vector_index_old);
	dma_get_patch(patch_old_address, patch_w, patch_h, patch_old);
	spu_patch_id_vector[0] = id_vector_index_old;

	int **min_borders_for_ppu = malloc_align(patch_no-1 * sizeof(int*), 4);

	for (i = 1 ; i < patch_no; i++) {
		patch_curr_address = get_random_patch_address(all_patches, total_nr_patches, &id_vector_index_curr);
		dma_get_patch(patch_curr_address, patch_w, patch_h, patch_curr);

		int *min_border = min_error_border(error_matrix, min_border_matrix, patch_old, patch_curr, patch_w, patch_h, overlap_size);
		min_borders_for_ppu[i-1] = min_border;
		//printf("MIN_BORDER_FOR_PPU[%d]=%p\n", i-1, min_borders_for_ppu[i-1]);

		spu_patch_id_vector[i] = id_vector_index_curr;
		memcpy(patch_old, patch_curr, patch_w * patch_h * sizeof(pixel_t));
	}
	
	dma_send_patch_id_vector(ppu_id_patch_vector_address, spu_patch_id_vector, patch_no);
	dma_send_min_borders(ppu_min_borders_adress, min_borders_for_ppu, patch_no-1, overlap_size);

	free_align(spu_patch_id_vector);
	free_align(all_patches);
	free_align(patch_old);
	free_align(patch_curr);
	free_matrix(error_matrix, overlap_size);

	return 0;
}
