#include <stdint.h>
#include <stdlib.h>
#include "q3_stubs.h"

void R_LoadPCX(const char *name, unsigned char **pic, int *width, int *height);

/* The engine accepts up to 1023x1023, so a mutated header could make every exec allocate and
 * palette-convert a multi-megabyte image. Only the pixel count is capped: any width and height
 * below 1024 is fuzzed as long as the area is at most 524288 pixels (640x480 and 800x600 pass).
 * Dimensions of 1024 and above still reach the engine's own bad-dimension branch, which is cheap. */
#define PCX_MAX_PIXELS (1u << 19)

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	unsigned char *pic = NULL;
	int w = 0, h = 0;

	if (size >= 12) {
		unsigned pw = (uint16_t)(data[8] + (data[9] << 8) + 1);
		unsigned ph = (uint16_t)(data[10] + (data[11] << 8) + 1);

		if (pw < 1024 && ph < 1024 && pw * ph > PCX_MAX_PIXELS)
			return -1;
	}

	q3_in = data;
	q3_in_len = size;
	R_LoadPCX("fuzz.pcx", &pic, &w, &h);
	free(pic);
	return 0;
}
