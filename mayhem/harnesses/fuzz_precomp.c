#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../../code/qcommon/q_shared.h"
#include "../../code/botlib/l_script.h"
#include "../../code/botlib/l_precomp.h"
#include "q3_stubs.h"

/* C-preprocessor style source (#define/#if/#include/macro expansion) read to the end; the host
 * file system hook serves the fuzz input for the main file and a fixed small header for any #include */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (size < 1 || size > 1 << 20)
		return -1;
	q3_feed(data, size);

	source_t *source = LoadSourceFile("fuzz.c");
	if (source) {
		token_t token;
		for (int n = 0; n < 1 << 16 && PC_ReadToken(source, &token); n++)
			;
		FreeSource(source);
	}
	PC_RemoveAllGlobalDefines();
	return 0;
}
