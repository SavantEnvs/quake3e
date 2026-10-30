#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int32_t puff(uint8_t *dest, uint32_t *destlen, uint8_t *source, uint32_t *sourcelen);

/* first two bytes pick the output capacity (up to 64 KiB), the rest is the deflate stream */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (size < 3)
		return -1;
	uint32_t destlen = ((uint32_t)data[0] | ((uint32_t)data[1] << 8)) + 1;
	uint32_t srclen = (uint32_t)(size - 2);
	uint8_t *src = (uint8_t *)malloc(srclen);
	uint8_t *dst = (uint8_t *)malloc(destlen);

	memcpy(src, data + 2, srclen);
	puff(dst, &destlen, src, &srclen);
	free(src);
	free(dst);
	return 0;
}
