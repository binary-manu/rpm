#ifndef _MBR_H_
#define _MBR_H_

namespace MBR
{
	extern "C" int StoreMBR(const char *path, const void *buffer);
	extern "C" int LoadMBR(const char *path, void *buffer);
}

#endif