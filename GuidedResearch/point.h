#pragma once

#include <GL\glew.h>
#include <math.h>

// Used to deal with floating point precision issues.
GLfloat epsilon = .000001f;

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

	inline bool operator<(const Point& rhs)
	{
		if (x < rhs.x && y < rhs.y)
		{
			return true;
		}

		return false;
	}
};
