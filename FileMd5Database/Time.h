#pragma once

#include <ctime>

namespace Time
{
	void Gmt(tm* gmt, time_t const* time);

	void Local(tm* local, time_t const* time);
}
