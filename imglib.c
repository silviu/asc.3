#include "imglib.h"
#include <string.h>

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
    for (i = 0; i < img->width * img->height; i++)
        free(img->buf[i]);
    free(img->buf);
    free(img);
}

image read_ppm(FILE * f)
{
    image img;
    unsigned int w, h;
    char a;
    int b;

    if (f == NULL) {
        perror("Error opening input file.\n");
        return NULL;
    }

    fscanf(f, "%c%d\n", &a, &b);
    fscanf(f, "%u %u\n", &w, &h);
    printf("%d %d\n", w, h);
    fscanf(f, "%d\n", &b);

    fpos_t curr_pos;
    fgetpos(f, &curr_pos);
    fseek (f , SEEK_CUR, SEEK_END);
    long lSize = ftell (f);
    fsetpos(f, &curr_pos);


    char* buf = malloc(lSize * sizeof(char));

    img = alloc_img(w, h);
    if (img != NULL) {
        size_t rd = fread(buf, 1, lSize, f);
        if (rd < w * h) {
            free_img(img);
            return NULL;
        }
        //printf("%s\n", buf);
        char *tok = strtok(buf, "\n");
        unsigned int i = 0, j = 0;
        while (tok != NULL)
        {
            if (j == 3) {
                j = 0;
                i++;
            }
            printf("%d\n", atoi(tok));
            img->buf[i][j] = atoi(tok);
            tok = strtok(NULL, "\n");
            j++;
            
        }
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
