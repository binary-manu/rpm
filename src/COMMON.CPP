#include "COMMON.H"

char Common::Bin2BCD(char bin)
{
	return bin % 10 + bin / 10 * 16;
}

char Common::BCD2Bin(char bcd)
{
	return (bcd & 0xF) + ((bcd & 0xF0) >> 4) * 10;
}

char Common::Hour12To24(char h)
{
	bool pm = h & 0x80;
	h &= ~0x80;
	
	if (pm)
		h = (h + 12) % 12;
	return h;
}

char Common::Hour24To12(char h)
{
	char tmp = h % 12;
	if (!tmp)
		tmp = 12;
	if (h >= 12)
		tmp |= 0x80;
	return tmp;
}

int Common::dow(int y, int m, int d)
{
	static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
	y -= m < 3;
	return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}