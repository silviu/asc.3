#include <stdio.h>
#include <spu_mfcio.h>
#include "imglib.h"

#define waitag(t) mfc_write_tag_mask(1<<t); mfc_read_tag_status_all();

void get_patch_info(int *patch_w, int *patch_h, int *patch_no, pixel_t** output_zone_start)
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
	*output_zone_start = (pixel_t*)spu_read_in_mbox();
}

void dma_get_patch_from_ppu(image patch)
{
	//get start of my zone
	while (spu_stat_in_mbox()<=0);
	pixel_t *zone_start = (pixel_t*)spu_read_in_mbox();

	uint32_t tag_id = mfc_tag_reserve();
	if (tag_id==MFC_TAG_INVALID) {
		printf("SPU: ERROR can't allocate tag ID\n"); 
		return NULL;
	}
	//printf("ZONE START: %p %p\n", zone_start, patch->buf);
	mfc_get((void *)(patch->buf), (void*)zone_start, (uint32_t)patch->width * patch->height * 3 *sizeof(unsigned char), tag_id, 0, 0);
	waitag(tag_id);
}



int main(unsigned long long speid, unsigned long long argp, unsigned long long envp)
{
	int patch_w, patch_h, patch_no;
	pixel_t *output_zone_start;
	unsigned int id = (unsigned int) argp;
	printf("SPU: SPU %d STARTED\n", id);
	
	get_patch_info(&patch_w, &patch_h, &patch_no, &output_zone_start);
	printf("SPU: P_W P_H P_NO: %d %d %d\n", patch_w, patch_h, patch_no);
	printf("PIXEL: %p\n", output_zone_start);

	image patch = alloc_img(patch_w, patch_h);
	printf("SPU %d: PATCH ADDRESS: %p\n", id, patch);
	dma_get_patch_from_ppu(patch);
	free_img(patch);

	return 0;
}
