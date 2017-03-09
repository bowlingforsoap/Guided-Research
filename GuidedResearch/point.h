#pragma once

#include <GL\glew.h>
#include <math.h>

// Used to deal with floating point precision issues.
const GLfloat epsilon = .000001f;

// Utility value
const GLfloat dummyValue = 666.f;

// An abstraction over two GLfloat-s with useful utility operators.
struct Point
{
	GLfloat x;
	GLfloat y;

	inline bool operator==(const Point& rhs)
	{
		if (x == rhs.x && y == rhs.y)
		{
			return true;
		}
		if (Point{ abs(x - rhs.x), abs(y - rhs.y) } < Point{ epsilon, epsilon })
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	inline bool operator!=(const Point& rhs) {
		return !operator==(rhs);
	}

	inline Point operator-(const Point& rhs)
	{
		return{ x - rhs.x, y - rhs.y };
	}

	inline Point operator-(const Point& rhs) const
	{
		return{ x - rhs.x, y - rhs.y };
	}

	inline bool operator<(const Point& rhs)
	{
		if (x < rhs.x && y < rhs.y)
		{
			return true;
		}

		return false;
	}

	/*inline Point operator+(const Point& rhs) {
		return Point{ x + rhs.x, y + rhs.y };
	}*/
};
