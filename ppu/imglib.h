#ifndef _IMGLIB_0
#define _IMGLIB_0

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>
#include <sys/queue.h>

// r, g, b
typedef unsigned char* pixel_t;

typedef struct {
    unsigned int width;
    unsigned int height;
    pixel_t *buf;
} image_t;

typedef image_t *image;

image alloc_img(unsigned int width, unsigned int height);
void free_img(image);
void apply_patch(image img_src, image img_dst, int x_index, int y_index, int patch_w, int patch_h);
image get_patch(image img, int patch_w, int patch_h, int x_index, int y_index);
image get_random_patch(image img, int patch_w, int patch_h);
void get_random_patch_indexes(image img, int patch_w, int patch_h, int *ret_x, int *ret_y);
image read_ppm(char *fis_in);
void write_ppm(char * fis_out, image img);

#define GET_PIXEL(IMG, X, Y) (IMG->buf[ ((Y) * IMG->width + (X)) ])
#define R(IMG, X, Y) (GET_PIXEL(IMG, X, Y)[0])
#define G(IMG, X, Y) (GET_PIXEL(IMG, X, Y)[1])
#define B(IMG, X, Y) (GET_PIXEL(IMG, X, Y)[2])
#endif
