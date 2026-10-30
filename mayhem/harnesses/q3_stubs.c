/* Host-environment stand-ins for the engine services the fuzzed modules import
 * (renderer refimport, botlib import, Com_*). Memory/FS only; no engine code is replaced. */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../code/qcommon/q_shared.h"
#include "../../code/renderercommon/tr_public.h"
#include "../../code/botlib/botlib.h"
#include "../../code/botlib/l_memory.h"
#include "../../code/botlib/l_script.h"
#include "../../code/botlib/l_precomp.h"
#include "../../code/botlib/l_struct.h"
#include "../../code/botlib/l_libvar.h"
#include "../../code/botlib/l_utils.h"
#include "../../code/botlib/aasfile.h"
#include "../../code/botlib/be_aas.h"
#include "../../code/botlib/be_aas_funcs.h"
#include "../../code/botlib/be_interface.h"
#include "../../code/botlib/be_aas_def.h"
#include "q3_stubs.h"

const uint8_t *q3_in;
size_t q3_in_len;

static void **hunk_blocks;
static size_t hunk_n, hunk_cap;

void q3_hunk_reset(void)
{
	for (size_t i = 0; i < hunk_n; i++)
		free(hunk_blocks[i]);
	hunk_n = 0;
}

static void *stub_hunk_alloc(size_t size)
{
	void *p = calloc(1, size ? size : 1);
	if (!p)
		abort();
	if (hunk_n == hunk_cap) {
		hunk_cap = hunk_cap ? hunk_cap * 2 : 64;
		hunk_blocks = (void **)realloc(hunk_blocks, hunk_cap * sizeof(void *));
		if (!hunk_blocks)
			abort();
	}
	hunk_blocks[hunk_n++] = p;
	return p;
}

/* ---- Com_* ---- */
void QDECL Com_Error(errorParm_t level, const char *fmt, ...)
{
	(void)level; (void)fmt;
	abort();
}

void QDECL Com_Printf(const char *fmt, ...)
{
	(void)fmt;
}

/* ---- renderer import ---- */
static void QDECL stub_ri_printf(printParm_t level, const char *fmt, ...)
{
	(void)level; (void)fmt;
}

static void NORETURN QDECL stub_ri_error(errorParm_t level, const char *fmt, ...)
{
	(void)level; (void)fmt;
	abort();
}

static void *stub_ri_malloc(size_t n) { void *p = malloc(n ? n : 1); if (!p) abort(); return p; }
static void stub_ri_free(void *p) { free(p); }

static int stub_ri_readfile(const char *name, void **buf)
{
	(void)name;
	if (buf) {
		uint8_t *copy = (uint8_t *)malloc(q3_in_len + 1);
		if (!copy)
			abort();
		memcpy(copy, q3_in, q3_in_len);
		copy[q3_in_len] = 0;
		*buf = copy;
	}
	return (int)q3_in_len;
}

static void stub_ri_freefile(void *buf) { free(buf); }

refimport_t ri = {
	.Printf = stub_ri_printf,
	.Error = stub_ri_error,
	.Malloc = stub_ri_malloc,
	.Free = stub_ri_free,
	.FS_ReadFile = stub_ri_readfile,
	.FS_FreeFile = stub_ri_freefile,
};

/* ---- botlib import ---- */
static void QDECL stub_bot_print(int type, const char *fmt, ...)
{
	(void)fmt;
	if (type == PRT_EXIT)
		abort();
}

static void *stub_bot_getmem(size_t n) { void *p = malloc(n ? n : 1); if (!p) abort(); return p; }
static void stub_bot_freemem(void *p) { free(p); }
static int stub_bot_avail(void) { return 1 << 24; }

static size_t bot_pos;
static const uint8_t *bot_cur;
static size_t bot_cur_len;
static int bot_root_served;

/* a stand-in for a header that exists on disk: any file other than the root one */
static const char bot_header[] = "#define Q3_HDR 1\n#ifndef Q3_HDR_H\n#define Q3_HDR_H\n#endif\n";

void q3_feed(const uint8_t *data, size_t len)
{
	q3_in = data;
	q3_in_len = len;
	bot_root_served = 0;
}

/* The fuzz input is the root file only (the first open after q3_feed). Every later open, i.e. an
 * #include, gets the fixed header: serving the input again would let it include itself without
 * bound, which no real file system allows for an invented path. */
static int stub_bot_fopen(const char *qpath, fileHandle_t *file, fsMode_t mode)
{
	(void)qpath;
	if (mode != FS_READ) {
		*file = 0;
		return -1;
	}
	bot_pos = 0;
	*file = 1;
	if (!bot_root_served) {
		bot_root_served = 1;
		bot_cur = q3_in;
		bot_cur_len = q3_in_len;
	} else {
		bot_cur = (const uint8_t *)bot_header;
		bot_cur_len = sizeof(bot_header) - 1;
	}
	return (int)bot_cur_len;
}

static int stub_bot_read(void *buffer, int len, fileHandle_t f)
{
	(void)f;
	size_t left = bot_cur_len - bot_pos;
	size_t n = (size_t)len < left ? (size_t)len : left;
	memcpy(buffer, bot_cur + bot_pos, n);
	bot_pos += n;
	return (int)n;
}

static int stub_bot_seek(fileHandle_t f, long offset, fsOrigin_t origin)
{
	(void)f;
	if (origin != FS_SEEK_SET || offset < 0)
		return -1;
	bot_pos = (size_t)offset > bot_cur_len ? bot_cur_len : (size_t)offset;
	return 0;
}

static void stub_bot_close(fileHandle_t f) { (void)f; }

botlib_import_t botimport = {
	.Print = stub_bot_print,
	.GetMemory = stub_bot_getmem,
	.FreeMemory = stub_bot_freemem,
	.AvailableMemory = stub_bot_avail,
	.HunkAlloc = stub_hunk_alloc,
	.FS_FOpenFile = stub_bot_fopen,
	.FS_Read = stub_bot_read,
	.FS_FCloseFile = stub_bot_close,
	.FS_Seek = stub_bot_seek,
};

/* the AAS world state and error printer live in be_aas_main.c, which drags in the whole bot AI */
aas_t aasworld;

void QDECL AAS_Error(char *fmt, ...)
{
	(void)fmt;
}
