#include "imglib.h"
#include <string.h>
#include <time.h>


image alloc_img(unsigned int width, unsigned int height)
{
    int i;
    image img;
    img = malloc(sizeof (image_t));
    img->buf = malloc((width * height + 1) * sizeof (pixel_t));
    for (i = 0; i < (width * height + 1); i++)
        img->buf[i] = malloc(3 * sizeof(char));
    img->width = width;
    img->height = height;
    return img;
}

void free_img(image img)
{
    int i;
    for (i = 0; i < (img->width * img->height + 1); i++)
        free(img->buf[i]);
    free(img->buf);
    free(img);
}

image get_patch(image img, int patch_w, int patch_h, int x_index, int y_index)
{
    int i ,j ;
    printf("HELOOOOOOOOOOOOOO\n");
    printf("%d %d\n", patch_w, patch_h);
    printf("%d %d\n", x_index, y_index);

    image patch = alloc_img(patch_w, patch_h);
    for (i = x_index; i < x_index+patch_w; i++)
        for (j = y_index; j < y_index+patch_h; j++) {
            printf("[PATCH] x = %d y = %d act_pos = %d\n", i-x_index, j-y_index, ((j-y_index)*img->width) + i-x_index);
            printf("[ IMG ] x = %d y = %d\n", i, j);
            memcpy(GET_PIXEL(patch, i-x_index, j-y_index), GET_PIXEL(img, i, j), 3* sizeof(unsigned char));
        }
    return patch;
}

image get_random_patch(image img, int patch_w, int patch_h)
{
    srand((unsigned)time(NULL));
    int max_rand_x = img->width  - patch_w;
    int max_rand_y = img->height - patch_h;

    int x_index = rand() % max_rand_x;
    int y_index = rand() % max_rand_y;

    image patch = get_patch(img, patch_w, patch_h, x_index, y_index);
    return patch;
}

image read_ppm(FILE * f)
{
    image img;
    unsigned int w, h;
    char a;
    int b;


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
        return img;
    }
    return img;
}

void
write_ppm(FILE * fd, image img) {
    unsigned int n;
    (void) fprintf(fd, "P5\n%d %d\n255\n", img->width, img->height);
    n = img->width * img->height;
    (void) fwrite(img->buf, sizeof (pixel_t), n, fd);
    (void) fflush(fd);
}
