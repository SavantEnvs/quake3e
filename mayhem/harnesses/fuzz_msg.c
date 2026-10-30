#include <stdint.h>
#include <string.h>
#include "../../code/qcommon/q_shared.h"
#include "../../code/qcommon/qcommon.h"

/* byte 0 = transport mode, the rest is the packet; the reader ops are chosen from the packet itself */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	static byte buf[MAX_MSGLEN];
	msg_t msg;
	usercmd_t from, to;

	if (size < 2 || size > sizeof(buf) + 1)
		return -1;
	memcpy(buf, data + 1, size - 1);
	if (data[0] & 1)
		MSG_InitOOB(&msg, buf, sizeof(buf));
	else
		MSG_Init(&msg, buf, sizeof(buf));
	msg.cursize = (int)(size - 1);
	MSG_BeginReading(&msg);
	memset(&from, 0, sizeof(from));
	memset(&to, 0, sizeof(to));

	for (size_t i = 1; i < size && msg.readcount <= msg.cursize; i++) {
		switch (data[i] % 11) {
		case 0: (void)MSG_ReadChar(&msg); break;
		case 1: (void)MSG_ReadByte(&msg); break;
		case 2: (void)MSG_ReadShort(&msg); break;
		case 3: (void)MSG_ReadLong(&msg); break;
		case 4: (void)MSG_ReadFloat(&msg); break;
		case 5: (void)MSG_ReadString(&msg); break;
		case 6: (void)MSG_ReadBigString(&msg); break;
		case 7: (void)MSG_ReadStringLine(&msg); break;
		case 8: (void)MSG_ReadAngle16(&msg); break;
		case 9: (void)MSG_ReadEntitynum(&msg); break;
		default:
			MSG_ReadDeltaUsercmdKey(&msg, (int)data[i], &from, &to);
			from = to;
			break;
		}
	}
	return 0;
}
