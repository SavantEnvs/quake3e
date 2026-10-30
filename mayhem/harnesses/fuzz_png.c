#include <stdint.h>
#include <stdlib.h>
#include "q3_stubs.h"

void R_LoadPNG(const char *name, unsigned char **pic, int *width, int *height);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	unsigned char *pic = NULL;
	int w = 0, h = 0;

	q3_in = data;
	q3_in_len = size;
	R_LoadPNG("fuzz.png", &pic, &w, &h);
	free(pic);
	return 0;
}
