#ifndef Q3_STUBS_H
#define Q3_STUBS_H
#include <stddef.h>
#include <stdint.h>

/* current fuzz input, served to the engine's file-system hooks */
extern const uint8_t *q3_in;
extern size_t q3_in_len;

/* set the fuzz input; the next botlib file open serves it, later opens serve a fixed header */
void q3_feed(const uint8_t *data, size_t len);

/* botlib "hunk" blocks are never freed by botlib; the harness releases them per exec */
void q3_hunk_reset(void);
#endif
