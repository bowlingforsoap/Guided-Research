#pragma once

#define _USE_MATH_DEFINES

#include <vector>
#include <gl\glew.h>
#include <math.h> /* atan2, abs */

#include "point.h"

using namespace std;

class Labeler {
public:
	static vector<vector<float>> computeAngles(const vector<vector<Point>>& contour) {
		vector<vector<float>> angles;
		angles.reserve(contour.size());


		for (const vector<Point>& contourLine : contour) {
			vector<float> contourLineAngles;
			contourLineAngles.reserve(contourLine.size());
			Point pointA, pointB, pointC;

			for (int i = 1; i < contourLine.size() - 1; i++) {
				pointA = contourLine[i - 1];
				pointB = contourLine[i];
				pointC = contourLine[i + 1];

				contourLineAngles.push_back(deltaAngleBetweenLines(pointA, pointB, pointC));
			}

			// Store the angle for the last.. 
			if (abs(abs(pointC.x) - 1.f) < epsilon || abs(abs(pointC.y) - 1.f) < epsilon) {
				// Store dummyValue if we are at the edge
				contourLineAngles.push_back(dummyValue);
			}
			else {
				pointA = contourLine[0];
				contourLineAngles.push_back(deltaAngleBetweenLines(pointB, pointC, pointA));
			}
			// ..and the first point
			pointB = contourLine[0];
			if (abs(abs(pointB.x) - 1.f) < epsilon || abs(abs(pointB.y) - 1.f) < epsilon) {
				contourLineAngles.insert(contourLineAngles.begin(), dummyValue);
			}
			else {
				pointA = contourLine[contourLine.size() - 1];
				pointC = contourLine[1];
				contourLineAngles.insert(contourLineAngles.begin(), deltaAngleBetweenLines(pointA, pointB, pointC));
			}

			angles.push_back(contourLineAngles);
		}

		return angles;
	}

private:
	// Computes an angle between two lines in anti-clockwise direction, returns it's absolute value.
	// Consider AB a vector that gives the direction of a 0 curvature (0 delta angle). Now delta angle would be how much BC bends from this perfect 0 curvature, without taking bending direction into account.
	static inline float deltaAngleBetweenLines(Point& pointA, Point& pointB, Point& pointC) {
		float angle;

		Point vecAB = pointB - pointA;
		Point vecBC = pointC - pointB;

		// Get counter clockwise angle from vecBC to vecAB (or CBA - 180)
		angle = (atan2(vecAB.y, vecAB.x) - atan2(vecBC.y, vecBC.x)) * 180.f / M_PI;
		/*if (angle < 0) {
			angle += 180.f;
		}*/

		return abs(angle);
	}
};