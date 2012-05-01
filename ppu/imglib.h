#ifndef _IMGLIB_0
#define _IMGLIB_0

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>
#include <sys/queue.h>

// r, g, b
struct pixel_t{
	char r;
	char g;
	char b;
};

typedef struct pixel_t pixel_t;

typedef struct {
    unsigned int width;
    unsigned int height;
    pixel_t *buf;
} image_t;

typedef image_t *image;

image alloc_img(unsigned int width, unsigned int height);
void free_img(image);
image read_ppm(char *fis_in);
void write_ppm(char * fis_out, image img);

#define GET_PIXEL(IMG, X, Y) (IMG->buf[ ((X) * IMG->height + (Y)) ])
#define R(IMG, X, Y) (GET_PIXEL(IMG, X, Y).r)
#define G(IMG, X, Y) (GET_PIXEL(IMG, X, Y).g)
#define B(IMG, X, Y) (GET_PIXEL(IMG, X, Y).b)
#endif
