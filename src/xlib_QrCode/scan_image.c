#include <stdio.h>
#include <stdlib.h>
#include <zbar.h>

zbar_image_scanner_t *scanner = NULL;


void get_yuv_data(const char *name, int *width, int *height, void **raw)
{
    FILE *pfile = fopen(name, "rb");
    if(!pfile) exit(2);
    *raw = malloc((*width) * (*height));
    fread(*raw,1,(*width)*(*height),pfile);
    fclose(pfile);
}

int main (int argc, char **argv)
{
    if(argc < 2) return(1);

    /* create a reader */
    scanner = zbar_image_scanner_create();

    /* configure the reader */
    zbar_image_scanner_set_config(scanner, 0, ZBAR_CFG_ENABLE, 1);

    /* obtain image data */
    int width = 1920, height = 1080;
    void *raw = NULL;
    get_yuv_data(argv[1], &width, &height, &raw);

    /* wrap image data */
    zbar_image_t *image = zbar_image_create();
    zbar_image_set_format(image, *(int*)"Y800");
    zbar_image_set_size(image, width, height);
    zbar_image_set_data(image, raw, width * height, zbar_image_free_data);

    /* scan the image for barcodes */
    int n = zbar_scan_image(scanner, image);
    if(0==n||-1==n)
    {
      printf("no symbols were found or -1 if an error occurs\n");
    }
    /* extract results */
    const zbar_symbol_t *symbol = zbar_image_first_symbol(image);
    for(; symbol; symbol = zbar_symbol_next(symbol)) {
        /* do something useful with results */
        zbar_symbol_type_t typ = zbar_symbol_get_type(symbol);
        const char *data = zbar_symbol_get_data(symbol);
        printf("decoded %s symbol \"%s\"\n",
               zbar_get_symbol_name(typ), data);
    }

    /* clean up */
    zbar_image_destroy(image);
    zbar_image_scanner_destroy(scanner);

    return(0);
}
