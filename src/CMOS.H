#ifndef _CMOS_H_
#define _CMOS_H_

#include <stddef.h>
#include "COMMON.H"

namespace CMOS
{
	using namespace Common;
	
	enum RTCTimeSource {RTC_USE_SAVED, RTC_KEEP_CMOS, RTC_USE_USERDATA};
	
	// A "structified" vision of the first 12 bytes stored in the RTC CMOS
	// memory
	#pragma(push, 1)
	struct RTCMap
	{
		char mSeconds;
		char mSecondsAlarm;
		char mMinutes;
		char mMinutesAlarm;
		char mHours;
		char mHoursAlarm;
		char mDOW;
		char mDay;
		char mMonth;
		char mYear;
		char mRegA;
		char mRegB;
	};
	#pragma pack()
	
	class RTCTime {
	public:
		enum HourRange {HourRange_12, HourRange_24};
		enum Format {Format_BCD, Format_Binary};
		static const char NoAlarm;
		
		
		
		HourRange GetRange() const;
		Format GetFormat() const;
		void SetFormat(Format value);
		void SetRange(HourRange value);
		
		char GetSeconds() const;
		char GetMinutes() const;
		char GetHour() const;
		char GetDay() const;
		char GetMonth() const;
		char GetYear() const;
		char GetDOW() const;
		char GetSecondsAlarm() const;
		char GetMinutesAlarm() const;
		char GetHourAlarm() const;
		void SetSeconds(char value);
		void SetMinutes(char value);
		void SetHour(char value);
		void SetSecondsAlarm(char value);
		void SetMinutesAlarm(char value);
		void SetHourAlarm(char value);
		void SetDay(char value);
		void SetMonth(char value);
		void SetYear(char value);
		
		// Creates a new RTCTime object from the given time and date, and
		// returns them using the specified range and format.
		RTCTime(Format fmt = Format_BCD, HourRange range = HourRange_24,
			char sec = 0, char min = 0, char hour = 0,
			char day = 1, char month = 1, char year = 0,
			char secAlm = 0, char minAlm = 0, char hourAlm = 0);
		// Creates a new object from a CMOS data block but takes the hour
		// range and number format from the given parameters: this allows
		// the CMOS map be 10 bytes long, excluding control registers A and B.
		RTCTime(const RTCMap *map, Format fmt, HourRange range);
		// Just like RTCTime(const RTCMap*, FOrmat, HourRange), but takes
		// the range and the format from the map itself (control register B)
		RTCTime(const RTCMap *map);
		
		// Returns a representation of the RTC data as expected by the chip
		// itself. Note:
		// 1) Take a copy of the returned pointer as it is not guaranteed that
		//   the result is cached
		// 2) DO NOT free the returned pointer 
		const RTCMap* GetFlatData() const;
		
		static void CopyTimeDate(RTCTime &dst, const RTCTime& src);
		
	private:
		char GetHelper(char val) const;
		char SetHelper(char val) const;
		void CreateHelper(const RTCMap *map);
	
		void UpdateDOW();
		enum {BaseYear = 2000};
		HourRange mRange;
		Format mFormat;
		mutable RTCMap mMap;	// Always uses binary/24h
		mutable RTCMap mHWMap;	// Depends on the requested range/format
	};
	
	extern "C" int ReadCMOSData(void *buffer);
	extern "C" void WriteCMOSData(const void *buffer);
	void PatchCMOSTimeDate(void* cmosData, const CMOS::RTCTime& time);
	void CopyCMOSTimeDate(void* cmosData, const void* cmosSrc);
	int ReadPatchAndWriteCMOSData(
		const char *path,
		RTCTimeSource timeSource,
		RTCTime newTime = RTCTime()
	);
	bool ToTimeItem(const char* text, size_t len, char* value);
}

#endif