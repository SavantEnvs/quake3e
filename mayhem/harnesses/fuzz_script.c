#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../../code/qcommon/q_shared.h"
#include "../../code/botlib/l_script.h"

/* first byte selects the script flags, the rest is the script text */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (size < 2 || size > 1 << 20)
		return -1;
	int len = (int)(size - 1);
	char *text = (char *)malloc(len);
	memcpy(text, data + 1, len);

	script_t *script = LoadScriptMemory(text, len, "fuzz");
	if (script) {
		token_t token;
		SetScriptFlags(script, data[0] & 0x7f);
		for (int n = 0; n < 1 << 16 && PS_ReadToken(script, &token); n++) {
			if (token.type == TT_STRING)
				StripDoubleQuotes(token.string);
			else if (token.type == TT_LITERAL)
				StripSingleQuotes(token.string);
			else if (token.type == TT_PUNCTUATION)
				(void)PunctuationFromNum(script, token.subtype);
		}
		FreeScript(script);
	}
	free(text);
	return 0;
}
