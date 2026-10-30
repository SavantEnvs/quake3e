#include <stdint.h>
#include <string.h>
#include "../../code/qcommon/q_shared.h"
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
#include "q3_stubs.h"

int AAS_LoadAASFile(char *filename);
void AAS_DumpAASData(void);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	aas_header_t hdr;

	if (size < sizeof(hdr))
		return -1;
	/* a lump longer than the file can't be read; the engine's hunk would refuse such sizes */
	memcpy(&hdr, data, sizeof(hdr));
	for (int i = 0; i < AAS_LUMPS; i++) {
		if ((unsigned)hdr.lumps[i].filelen > size)
			return -1;
	}

	q3_feed(data, size);
	AAS_LoadAASFile((char *)"maps/fuzz.aas");
	AAS_DumpAASData();
	memset(&aasworld, 0, sizeof(aasworld));
	q3_hunk_reset();
	return 0;
}
