#ifndef CORELIB_PERFORMANCE_COUNTER_H
#define CORELIB_PERFORMANCE_COUNTER_H

#include "Common.h"

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace CoreLib
{
	namespace Diagnostics
	{
		typedef long long TimePoint;
		typedef long long Duration;
		class PerformanceCounter
		{
			static TimePoint frequency;
		public:
			static inline TimePoint Start() 
			{
				TimePoint rs;
				QueryPerformanceCounter((LARGE_INTEGER*)&rs);
				return rs;
			}
			static inline Duration End(TimePoint counter)
			{
				return Start() - counter;
			}
			static inline float EndSeconds(TimePoint counter)
			{
				return (float)ToSeconds(Start() - counter);
			}
			static inline double ToSeconds(Duration duration)
			{
				if (frequency == 0)
					QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
				auto rs = duration / (double)frequency;
				return rs;
			}
		};
	}
}

#endif