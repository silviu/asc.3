#include "imglib.h"

int main(int argc, char **argv)
{
	char *fis_in, *fis_out;
	int zoom, nr_bucati_dim1, nr_bucati_dim2, 
	banda_de_suprapunere_dim1, banda_de_suprapunere_dim2;
	int patch_w, patch_h;

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

	FILE *f = fopen(fis_in, "r");
    if (f == NULL) {
        perror("Error opening input file");
        return -1;
    }

	image img = read_ppm(f);
	if (img == NULL) {
		fprintf(stderr, "Error reading image file.\n");
		return -1;
	}

	patch_w = (zoom * img->width)/nr_bucati_dim1;
	patch_h = (zoom * img->height)/nr_bucati_dim2;

	patch_w = 3;
	patch_h = 3;

	printf("PATCH SIZES= %d %d\n", patch_w, patch_h);
	image patch = get_random_patch(img, patch_w, patch_h);
	int i;
	for(i = 0; i < patch_w * patch_h; i++)
		printf("%d %d %d\n", patch->buf[i][0], patch->buf[i][1], patch->buf[i][2]);
	
	fclose(f);
	free_img(img);
	return 0;
}