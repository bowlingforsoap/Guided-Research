#pragma once

#include "point.h"

struct Line
{
	Point begin;
	Point end;

	inline bool operator==(const Line& rhs)
	{
		return (begin == rhs.begin) && (end == rhs.end);
	}

	inline bool operator!=(const Line& rhs)
	{
		return !operator==(rhs);
	}
};