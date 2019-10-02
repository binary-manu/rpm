#ifndef _MBR_H_
#define _MBR_H_

#include <stddef.h>

namespace MBR
{
	const size_t MBR_BUFFER_SIZE = 512;
	extern "C" int StoreMBR(const char *path, const void *buffer);
	extern "C" int LoadMBR(const char *path, void *buffer);
}

#endif