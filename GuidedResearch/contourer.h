#pragma once

#include <gl\glew.h>
#include <vector>

using namespace std;

struct Point {
	GLfloat x;
	GLfloat y;

	inline bool operator==(const Point& rhs) {
		if (x == rhs.x && y == rhs.y) {
			return true;
		}
		else {
			return false;
		}
	}
};

struct Line {
	Point begin;
	Point end;
};

// All lines that belong to this contour.
vector<vector<Point>> getContour(GLvoid* feedback, int numPrimitives) {
	// Final result container
	vector<vector<Point>> contour;
	// Complete contour lines assembled from primitives (small line pieces) stored in feedback
	vector<Point> contourLine;

	// Get data from feedback
	vector<GLfloat> feedbackVector;
	// Primitive is a line: 2 verts with 2 floats per vertex
	feedbackVector.resize(numPrimitives * 2 * 2);
	// Copy data from feedback buffer to feedbackVector for future manipulations
	memcpy(&feedbackVector[0], feedback, feedbackVector.size() * sizeof(GLfloat));


	while (feedbackVector.size() >= 4) {
		contourLine.clear();

		// Get last 4 elements
		int lastEl = feedbackVector.size() - 1;
		Line l1 = { Point{feedbackVector[lastEl - 3], feedbackVector[lastEl - 2]}, Point{ feedbackVector[lastEl - 1], feedbackVector[lastEl]} };

		feedbackVector._Pop_back_n(4);

		contourLine.push_back(l1.begin);
		contourLine.push_back(l1.end);

		int i = 0;
		// Try to append lines to the back of l1
		while (i <= feedbackVector.size() - 4) {
			// TODO: unravel the mystery
			if (feedbackVector.size() == 0) {
				break;
			}

			Line l2 = { Point{feedbackVector[i], feedbackVector[i + 1]}, Point{ feedbackVector[i + 2], feedbackVector[i + 3]} };

			// If l2 continues l1
			if (l1.end == l2.begin) {
				// Add it l2 to the back of contourLine
				contourLine.push_back(l2.end);
				l1.end = l2.end;
			}
			else if (l1.begin == l2.end) {
				// Add it l2 to the front of contourLine
				contourLine.insert(contourLine.begin(), l2.begin);
				l1.begin = l2.begin;
			}
			else {
				i += 4;
				continue;
			}

			// Start comparison from the begining again
			i = 0;
			// Remove l2 from the feedbackVector
			feedbackVector.erase(feedbackVector.begin() + i, feedbackVector.begin() + i + 4);
		}

		
		contour.push_back(contourLine);
	}

	return contour;
}

