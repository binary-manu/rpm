#include "CMOS.H"

void CMOS::PatchCMOSTimeDate(void* cmosData, const RTCTime& time)
{
	CopyCMOSTimeDate(cmosData, time.GetFlatData());
}

void CMOS::CopyCMOSTimeDate(void* cmosData, const void* cmosSrc)
{
	char *cmos = (char*)cmosData;
	char *src = (char*)cmosSrc;
	cmos[0] = src[0];
	cmos[2] = src[2];
	cmos[4] = src[4];
	cmos[6] = src[6];
	cmos[7] = src[7];
	cmos[8] = src[8];
	cmos[9] = src[9];
}

const char CMOS::RTCTime::NoAlarm = 0xC0;

char CMOS::RTCTime::GetHelper(char val) const
{
	return (GetFormat() == Format_BCD) ? Common::Bin2BCD(val) : val;
}
char CMOS::RTCTime::SetHelper(char val) const
{
	return (GetFormat() == Format_BCD) ? Common::BCD2Bin(val) : val;
}

void CMOS::RTCTime::SetFormat(Format value)
{
	mFormat = value;
}
void CMOS::RTCTime::SetRange(HourRange value)
{
	mRange = value;
}

CMOS::RTCTime::Format CMOS::RTCTime::GetFormat() const
{
	return mFormat;
}

CMOS::RTCTime::HourRange CMOS::RTCTime::GetRange() const
{
	return mRange;
}
		
char CMOS::RTCTime::GetSeconds() const
{
	return GetHelper(mMap.mSeconds);
}

char CMOS::RTCTime::GetMinutes() const
{
	return GetHelper(mMap.mMinutes);
}

char CMOS::RTCTime::GetHour() const
{
	char h = (GetRange() == HourRange_12) ? Common::Hour24To12(mMap.mHours) :
		mMap.mHours;
	return (h & 0x80) | GetHelper(h & ~0x80);
}

char CMOS::RTCTime::GetDay() const
{
	return GetHelper(mMap.mDay);
}

char CMOS::RTCTime::GetMonth() const
{
	return GetHelper(mMap.mMonth);
}

char CMOS::RTCTime::GetYear() const
{
	return GetHelper(mMap.mYear);
}

char CMOS::RTCTime::GetDOW() const
{
	return GetHelper(mMap.mDOW);
}

char CMOS::RTCTime::GetSecondsAlarm() const
{
	if (mMap.mSecondsAlarm == NoAlarm)
		return NoAlarm;
	return GetHelper(mMap.mSecondsAlarm);
}

char CMOS::RTCTime::GetMinutesAlarm() const
{
	if (mMap.mMinutesAlarm == NoAlarm)
		return NoAlarm;
	return GetHelper(mMap.mMinutesAlarm);
}

char CMOS::RTCTime::GetHourAlarm() const
{
	if (mMap.mHoursAlarm == NoAlarm)
		return NoAlarm;
	char h = (GetRange() == HourRange_12) ?
		Common::Hour24To12(mMap.mHoursAlarm) :	mMap.mHoursAlarm;
	return (h & 0x80) | GetHelper(h & ~0x80);
}
		
void CMOS::RTCTime::SetSeconds(char value)
{
	mMap.mSeconds = SetHelper(value);
}

void CMOS::RTCTime::SetMinutes(char value)
{
	mMap.mMinutes = SetHelper(value);
}

void CMOS::RTCTime::SetHour(char value)
{
	mMap.mHours = SetHelper(value);
}

void CMOS::RTCTime::SetSecondsAlarm(char value)
{
	mMap.mSecondsAlarm = (NoAlarm == value) ? NoAlarm : SetHelper(value);
}

void CMOS::RTCTime::SetMinutesAlarm(char value)
{
	mMap.mMinutesAlarm = (NoAlarm == value) ? NoAlarm : SetHelper(value);
}

void CMOS::RTCTime::SetHourAlarm(char value)
{
	mMap.mHoursAlarm = (NoAlarm == value) ? NoAlarm : SetHelper(value);
}

void CMOS::RTCTime::SetDay(char value)
{
	mMap.mDay = SetHelper(value);
	UpdateDOW();
}

void CMOS::RTCTime::SetMonth(char value)
{
	mMap.mMonth = SetHelper(value);
	UpdateDOW();
}

void CMOS::RTCTime::SetYear(char value)
{
	mMap.mYear = SetHelper(value);
	UpdateDOW();
}
		
CMOS::RTCTime::RTCTime(Format fmt, HourRange range,
		char sec, char min, char hour,
		char day, char month, char year,
		char secAlm, char minAlm, char hourAlm):
mFormat(fmt), mRange(range)
{
	SetSeconds(sec);
	SetMinutes(min);
	SetHour(hour);
	SetDay(day);
	SetMonth(month);
	SetYear(year);
	SetSecondsAlarm(secAlm);
	SetMinutesAlarm(minAlm);
	SetHourAlarm(hourAlm);
}
		

void CMOS::RTCTime::CreateHelper(const RTCMap *map)
{
	SetSeconds(map->mSeconds);
	SetMinutes(map->mMinutes);
	SetHour(map->mHours);
	SetDay(map->mDay);
	SetMonth(map->mMonth);
	SetYear(map->mYear);
	SetSecondsAlarm(map->mSecondsAlarm);
	SetMinutesAlarm(map->mMinutesAlarm);
	SetHourAlarm(map->mHoursAlarm);
}
		
CMOS::RTCTime::RTCTime(const RTCMap *map, Format fmt, HourRange range):
mFormat(fmt), mRange(range)
{
	CreateHelper(map);
}

CMOS::RTCTime::RTCTime(const RTCMap *map)
{
	mFormat = (map->mRegB & 4) ? Format_Binary : Format_BCD;
	mRange = (map->mRegB & 2) ? HourRange_24 : HourRange_12;
	CreateHelper(map);
}
		
const CMOS::RTCMap *CMOS::RTCTime::GetFlatData() const
{
	using namespace Common;
	mHWMap.mSeconds = GetSeconds();
	mHWMap.mSecondsAlarm = GetSecondsAlarm();
	mHWMap.mMinutes = GetMinutes();
	mHWMap.mMinutesAlarm = GetMinutesAlarm();
	mHWMap.mHours = GetHour();
	mHWMap.mHoursAlarm = GetHourAlarm();
	mHWMap.mDay = GetDay();
	mHWMap.mMonth = GetMonth();
	mHWMap.mYear = GetYear();
	mHWMap.mDOW = GetDOW();
	mHWMap.mRegB = ((GetFormat() == Format_Binary) << 2) | 
		((GetRange() == HourRange_24) << 1);
	return &mHWMap;
}
	
void CMOS::RTCTime::UpdateDOW()
{
	mMap.mDOW = 1 + dow(BaseYear + GetYear(), GetMonth(), GetDay());
}

void CMOS::RTCTime::CopyTimeDate(RTCTime &dst, const RTCTime& src)
{
	dst.SetSeconds(src.GetSeconds());
	dst.SetMinutes(src.GetMinutes());
	dst.SetHour(src.GetHour());
	dst.SetDay(src.GetDay());
	dst.SetMonth(src.GetMonth());
	dst.SetYear(src.GetYear());
}