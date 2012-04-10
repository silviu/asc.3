#include "imglib.h"

int main()
{
	int i;
	FILE *f = fopen("caramida.ppm", "r");
	image img = read_ppm(f);
	printf("SIZE=%d\n", img->width * img->height * 3);
	for (i = 0; i < img->height * img->width; i++)
		printf("%d %d %d\n", img->buf[i][0], img->buf[i][1], img->buf[i][2]);
	fclose(f);
	return 0;
}