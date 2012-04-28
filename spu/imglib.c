#include "imglib.h"
#include <string.h>
#include <time.h>
#include <libmisc.h>


image alloc_img(unsigned int width, unsigned int height)
{
    unsigned int i;
    image img;
    img = malloc_align(sizeof (image_t), 7);
    img->buf = malloc_align((width * height + 1) * sizeof (pixel_t), 7);
    for (i = 0; i < (width * height + 1); i++)
        img->buf[i] = malloc_align(3 * sizeof(unsigned char), 7);
    img->width = width;
    img->height = height;
    return img;
}

void free_img(image img)
{
    unsigned int i;
    for (i = 0; i < (img->width * img->height + 1); i++)
        free_align(img->buf[i]);
    free_align(img->buf);
    free_align(img);
}

void apply_patch(image img_src, image img_dst, int x_index, int y_index, int patch_w, int patch_h)
{
    int i, j;
    for (i = 0; i < patch_w; i++)
        for (j = 0; j < patch_h; j++)
            memcpy(GET_PIXEL(img_dst, j+x_index, i+y_index), GET_PIXEL(img_src, i, j), 3 * sizeof(unsigned char));
}

image get_patch(image img, int x_index, int y_index, int patch_w, int patch_h)
{
    int i, j ;
    //printf("%d %d\n", patch_w, patch_h);
    //printf("%d %d\n", x_index, y_index);

    image patch = alloc_img(patch_w, patch_h);
    for (i = x_index; i < x_index+patch_w; i++)
        for (j = y_index; j < y_index+patch_h; j++) {
            //printf("[PATCH] x = %d y = %d\n", i-x_index, j-y_index);
            //printf("[ IMG ] x = %d y = %d\n", i, j);
            memcpy(GET_PIXEL(patch, i-x_index, j-y_index), GET_PIXEL(img, i, j), 3* sizeof(unsigned char));
        }
    return patch;
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

image get_random_patch(image img, int patch_w, int patch_h)
{
    int x_index, y_index;
    get_random_patch_indexes(img, patch_w, patch_h, &x_index, &y_index);
    image patch = get_patch(img, x_index, y_index, patch_w, patch_h);
    return patch;
}

image read_ppm(char* fis_in)
{
    image img;
    unsigned int w, h;
    char a;
    int b;

    FILE *f = fopen(fis_in, "r");
    if (f == NULL) {
        perror("Error opening input file");
        return NULL;
    }

    fscanf(f, "%c%d\n", &a, &b);
    fscanf(f, "%u %u\n", &w, &h);
    fscanf(f, "%d\n", &b);

    fpos_t curr_pos;
    fgetpos(f, &curr_pos);
    fseek (f , SEEK_CUR, SEEK_END);
    long f_size = ftell (f);
    fsetpos(f, &curr_pos);


    char* buf = malloc(f_size * sizeof(char));

    img = alloc_img(w, h);
    if (img != NULL) {
        size_t rd = fread(buf, 1, f_size, f);
        if (rd < w * h) {
            free_img(img);
            fclose(f);
            return NULL;
        }
        char *tok = strtok(buf, "\n");
        unsigned int i = 0, j = 0;
        while (tok != NULL)
        {
            if (j == 3) {
                j = 0;
                i++;
            }
            img->buf[i][j] = atoi(tok);
            tok = strtok(NULL, "\n");
            j++;
            
        }
        free(buf);
        fclose(f);
        return img;
    }
    fclose(f);
    return img;
}

void write_ppm(char *fis_out, image img) 
{
    FILE *f = fopen(fis_out, "w");
    if (f == NULL) {
        perror("Error opening input file");
        return;
    }

    unsigned int i, j;
    (void) fprintf(f, "P3\n%d %d\n255\n", img->width, img->height);
    for (i = 0; i < img->width; i++)
        for (j = 0; j < img->height; j++) {
            fprintf(f, "%d\n", R(img, i, j));
            fprintf(f, "%d\n", G(img, i, j));
            fprintf(f, "%d\n", B(img, i, j));
        }
    fclose(f);
}
