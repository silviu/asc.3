#include "imglib.h"
#include <string.h>
#include <time.h>
#include <libmisc.h>

#define SPU_THREADS 8

image alloc_img(unsigned int width, unsigned int height)
{
    image img;
    img = malloc_align(sizeof (image_t), 4);
    img->buf = malloc_align((width * height + 1) * sizeof (pixel_t), 4);
    img->width = width;
    img->height = height;
    return img;
}

void free_img(image img)
{
    free_align(img->buf);
    free_align(img);
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
            if (j % 3 == 0)
                img->buf[i].r = atoi(tok);
            else if (j % 3 == 1)
                img->buf[i].g = atoi(tok);
            else
                img->buf[i].b = atoi(tok);
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
