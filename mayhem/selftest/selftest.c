/* Known-answer checks over the engine's untrusted-input parsers (image loaders, inflate, script
 * tokenizer, preprocessor, AAS loader, network message reader). Built with the project's normal
 * flags; test.sh requires the final "SELFTEST passed=N failed=0" line. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../code/qcommon/q_shared.h"
#include "../../code/qcommon/qcommon.h"
#include "../../code/qcommon/puff.h"
#include "../../code/botlib/l_memory.h"
#include "../../code/botlib/l_script.h"
#include "../../code/botlib/l_precomp.h"
#include "../../code/botlib/l_struct.h"
#include "../../code/botlib/l_libvar.h"
#include "../../code/botlib/l_utils.h"
#include "../../code/botlib/aasfile.h"
#include "../../code/botlib/botlib.h"
#include "../../code/botlib/be_aas.h"
#include "../../code/botlib/be_aas_funcs.h"
#include "../../code/botlib/be_interface.h"
#include "../../code/botlib/be_aas_def.h"
#include "../harnesses/q3_stubs.h"
#include "fixtures.h"

void R_LoadPNG(const char *name, byte **pic, int *width, int *height);
void R_LoadPCX(const char *name, byte **pic, int *width, int *height);
int AAS_LoadAASFile(char *filename);
void AAS_DumpAASData(void);

static int passed, failed;

#define CHECK(name, cond) do { \
	if (cond) { passed++; printf("PASS %s\n", name); } \
	else { failed++; printf("FAIL %s (line %d)\n", name, __LINE__); } \
} while (0)

static void feed(const void *p, size_t n) { q3_feed((const uint8_t *)p, n); }

static void test_png(void)
{
	static const byte want_rgba[16] = { 255, 0, 0, 255, 0, 255, 0, 128, 0, 0, 255, 255, 10, 20, 30, 40 };
	static const byte want_rgb[24] = { 0, 1, 2, 255, 3, 4, 5, 255, 6, 7, 8, 255,
	                                   100, 101, 102, 255, 103, 104, 105, 255, 106, 107, 108, 255 };
	byte *pic = NULL;
	int w = 0, h = 0;

	feed(fx_png_rgba, sizeof(fx_png_rgba));
	R_LoadPNG("a.png", &pic, &w, &h);
	CHECK("png rgba dims", w == 2 && h == 2 && pic);
	CHECK("png rgba pixels", pic && memcmp(pic, want_rgba, sizeof(want_rgba)) == 0);
	free(pic);

	pic = NULL; w = h = 0;
	feed(fx_png_rgb, sizeof(fx_png_rgb));
	R_LoadPNG("b.png", &pic, &w, &h);
	CHECK("png rgb dims", w == 3 && h == 2 && pic);
	CHECK("png rgb pixels", pic && memcmp(pic, want_rgb, sizeof(want_rgb)) == 0);
	free(pic);

	pic = NULL; w = h = 0;
	feed(fx_png_rgba, 20);
	R_LoadPNG("trunc.png", &pic, &w, &h);
	CHECK("png truncated rejected", pic == NULL);
	free(pic);
}

static void test_pcx(void)
{
	byte *pic = NULL;
	int w = 0, h = 0;

	feed(fx_pcx, sizeof(fx_pcx));
	R_LoadPCX("a.pcx", &pic, &w, &h);
	CHECK("pcx dims", w == 2 && h == 2 && pic);
	CHECK("pcx pixel0", pic && pic[0] == 1 && pic[1] == 2 && pic[2] == 3 && pic[3] == 255);
	CHECK("pcx pixel1", pic && pic[4] == 10 && pic[5] == 20 && pic[6] == 30);
	CHECK("pcx pixel3", pic && pic[12] == 7 && pic[13] == 8 && pic[14] == 9);
	free(pic);

	pic = NULL; w = h = 0;
	{
		byte bad[sizeof(fx_pcx)];
		memcpy(bad, fx_pcx, sizeof(bad));
		bad[0] = 0;
		feed(bad, sizeof(bad));
		R_LoadPCX("bad.pcx", &pic, &w, &h);
		CHECK("pcx bad manufacturer rejected", pic == NULL);
	}
}

static void test_inflate(void)
{
	static const char text[] = "quake3e quake3e quake3e: the quick brown fox";
	uint8_t out[128];
	uint8_t src[sizeof(fx_deflate)];
	uint32_t outlen = sizeof(out), srclen = sizeof(fx_deflate);
	int32_t rc;

	memcpy(src, fx_deflate, sizeof(src));
	rc = puff(out, &outlen, src, &srclen);
	CHECK("inflate rc", rc == 0);
	CHECK("inflate length", outlen == sizeof(text) - 1);
	CHECK("inflate bytes", outlen == sizeof(text) - 1 && memcmp(out, text, outlen) == 0);

	outlen = 8; srclen = sizeof(fx_deflate);
	memcpy(src, fx_deflate, sizeof(src));
	CHECK("inflate output overflow reported", puff(out, &outlen, src, &srclen) == 1);
}

static void test_script(void)
{
	static char text[] = "weapon \"rail gun\" { damage 100 speed 1.5 mask 0x1f }";
	script_t *s = LoadScriptMemory(text, (int)strlen(text), "kat");
	token_t t;

	CHECK("script loads", s != NULL);
	if (!s)
		return;
	CHECK("script name token", PS_ReadToken(s, &t) && t.type == TT_NAME && strcmp(t.string, "weapon") == 0);
	CHECK("script string token", PS_ReadToken(s, &t) && t.type == TT_STRING && strcmp(t.string, "\"rail gun\"") == 0);
	CHECK("script brace token", PS_ReadToken(s, &t) && t.type == TT_PUNCTUATION && t.subtype == P_BRACEOPEN);
	PS_ReadToken(s, &t);
	CHECK("script damage name", strcmp(t.string, "damage") == 0);
	CHECK("script int value", PS_ReadToken(s, &t) && t.type == TT_NUMBER && (t.subtype & TT_INTEGER) && t.intvalue == 100);
	PS_ReadToken(s, &t);
	CHECK("script speed value", PS_ReadToken(s, &t) && (t.subtype & TT_FLOAT) && fabsf(t.floatvalue - 1.5f) < 1e-6f);
	PS_ReadToken(s, &t);
	CHECK("script hex value", PS_ReadToken(s, &t) && (t.subtype & TT_HEX) && t.intvalue == 0x1f);
	CHECK("script close brace", PS_ReadToken(s, &t) && t.subtype == P_BRACECLOSE);
	CHECK("script end", !PS_ReadToken(s, &t) && EndOfScript(s));
	FreeScript(s);
}

static void test_precomp(void)
{
	static char text[] = "#define SQUARE(x) ((x)*(x))\n#define N 4\n#ifdef N\nSQUARE(N) + N\n#else\nnope\n#endif\n";
	source_t *src;
	feed(text, strlen(text));
	src = LoadSourceFile("kat.c");
	char joined[256] = "";
	token_t t;

	CHECK("precomp loads", src != NULL);
	if (!src)
		return;
	while (PC_ReadToken(src, &t)) {
		if (strlen(joined) + strlen(t.string) + 1 < sizeof(joined))
			strcat(joined, t.string);
	}
	CHECK("precomp macro expansion", strcmp(joined, "((4)*(4))+4") == 0);
	FreeSource(src);
	PC_RemoveAllGlobalDefines();

	/* an #include of any other path yields the fixed header; it must not re-serve the input */
	{
		static char inc[] = "#include \"fuzz.c\"\n#include <a/b.h>\nQ3_HDR\n";
		feed(inc, strlen(inc));
		src = LoadSourceFile("kat.c");
		joined[0] = 0;
		CHECK("precomp include loads", src != NULL);
		if (src) {
			while (PC_ReadToken(src, &t)) {
				if (strlen(joined) + strlen(t.string) + 1 < sizeof(joined))
					strcat(joined, t.string);
			}
			CHECK("precomp include header", strcmp(joined, "1") == 0);
			FreeSource(src);
		}
		PC_RemoveAllGlobalDefines();
	}
}

static void le32(unsigned char *p, int v) { p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24; }

static void test_aas(void)
{
	unsigned char *bad;
	int rc;

	feed(fx_aas, sizeof(fx_aas));
	rc = AAS_LoadAASFile((char *)"maps/kat.aas");
	CHECK("aas load rc", rc == BLERR_NOERROR);
	CHECK("aas loaded", aasworld.loaded == qtrue);
	CHECK("aas counts", aasworld.numareas == 2 && aasworld.numnodes == 2 && aasworld.numclusters == 2 &&
	      aasworld.numvertexes == 4 && aasworld.numedges == 5 && aasworld.reachabilitysize == 1);
	CHECK("aas plane", aasworld.numplanes == 1 && aasworld.planes[0].normal[2] == 1.0f && aasworld.planes[0].type == 2);
	CHECK("aas vertex", aasworld.vertexes[2][0] == 1.0f && aasworld.vertexes[2][1] == 1.0f);
	CHECK("aas area", aasworld.areas[1].numfaces == 1 && aasworld.areas[1].center[0] == 0.5f);
	CHECK("aas reachability", aasworld.reachability[0].areanum == 1 && aasworld.reachability[0].traveltime == 10);
	AAS_DumpAASData();
	memset(&aasworld, 0, sizeof(aasworld));
	q3_hunk_reset();

	/* corrupt the face -> plane reference: the validator must refuse the file */
	bad = (unsigned char *)malloc(sizeof(fx_aas));
	memcpy(bad, fx_aas, sizeof(fx_aas));
	{
		int faces_ofs = (int)(bad[12 + 5 * 8] | bad[12 + 5 * 8 + 1] << 8 | bad[12 + 5 * 8 + 2] << 16);
		le32(bad + faces_ofs + 24, 7); /* faces[1].planenum */
	}
	feed(bad, sizeof(fx_aas));
	rc = AAS_LoadAASFile((char *)"maps/bad.aas");
	CHECK("aas corrupt rejected", rc == BLERR_CANNOTREADAASLUMP && aasworld.loaded == qfalse);
	AAS_DumpAASData();
	memset(&aasworld, 0, sizeof(aasworld));
	q3_hunk_reset();
	free(bad);
}

static void test_msg(void)
{
	static byte buf[MAX_MSGLEN];
	static const byte oob_expect[] = { 0x04, 0x03, 0x02, 0x01, 0xfe, 0x34, 0x12, 'h', 'i', 0 };
	msg_t m;

	MSG_InitOOB(&m, buf, sizeof(buf));
	MSG_WriteLong(&m, 0x01020304);
	MSG_WriteByte(&m, 0xfe);
	MSG_WriteShort(&m, 0x1234);
	MSG_WriteString(&m, "hi");
	CHECK("msg oob wire bytes", m.cursize == (int)sizeof(oob_expect) && memcmp(buf, oob_expect, sizeof(oob_expect)) == 0);
	MSG_BeginReadingOOB(&m);
	CHECK("msg oob long", MSG_ReadLong(&m) == 0x01020304);
	CHECK("msg oob byte", MSG_ReadByte(&m) == 0xfe);
	CHECK("msg oob short", MSG_ReadShort(&m) == 0x1234);
	CHECK("msg oob string", strcmp(MSG_ReadString(&m), "hi") == 0);

	MSG_Init(&m, buf, sizeof(buf));
	MSG_WriteLong(&m, -123456);
	MSG_WriteByte(&m, 77);
	MSG_WriteString(&m, "huffman");
	MSG_WriteFloat(&m, 3.25f);
	MSG_BeginReading(&m);
	CHECK("msg huffman long", MSG_ReadLong(&m) == -123456);
	CHECK("msg huffman byte", MSG_ReadByte(&m) == 77);
	CHECK("msg huffman string", strcmp(MSG_ReadString(&m), "huffman") == 0);
	CHECK("msg huffman float", MSG_ReadFloat(&m) == 3.25f);
	CHECK("msg read past end", (MSG_ReadByte(&m), MSG_ReadLong(&m)) == -1);
}

int main(void)
{
	test_png();
	test_pcx();
	test_inflate();
	test_script();
	test_precomp();
	test_aas();
	test_msg();
	printf("SELFTEST passed=%d failed=%d\n", passed, failed);
	return failed ? 1 : 0;
}
