#include "Time.h"

namespace Time
{
	void Gmt(tm* const gmt, time_t const* const time)
	{
#if (defined _WIN32 || _WIN64)
		gmtime_s(gmt, time);
#else
		gmtime_r(time, gmt);
#endif
	}

	void Local(tm* const local, time_t const* const time)
	{
#if (defined _WIN32 || _WIN64)
		localtime_s(local, time);
#else
		localtime_r(time, local);
#endif
	}
}
