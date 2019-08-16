#ifndef _COMMON_H_
#define _COMMON_H_

namespace Common
{
	char Bin2BCD(char bin);
	char BCD2Bin(char bin);
	int dow(int y, int m, int d);
	char Hour12To24(char h);
	char Hour24To12(char h);
}

#endif
