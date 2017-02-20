#pragma once

#define _USE_MATH_DEFINES

#include <vector>
#include <gl\glew.h>
#include <math.h> /* atan2, abs */
#include <limits>

#include "point.h"

using namespace std;

class Labeler {
public:
	// Searches for a point sequence, which comprises a broken line with minimal curvature and is at least labelLength long.
	static vector<Point> findCandidatePositions(GLfloat labelLength, const vector<Point>& contourLine, const vector<GLfloat>& angles) {
		CandidatePosition candidatePosition;
		candidatePosition.curvature = numeric_limits<float>::max();
		candidatePosition.length = 0.f;
		
		for (int i = 0; i < contourLine.size(); i++) {
			CandidatePosition currentCandidate;
			currentCandidate.curvature = 0.f;
			currentCandidate.length = 0.f;
			currentCandidate.position.clear();

			currentCandidate.position.push_back(contourLine[i]);
			int j = (i == contourLine.size() - 1) ? 0 : i + 1;
			Point previousPoint = contourLine[i];//contourLine[ (j != 0) ? j - 1 : contourLine.size() - 1 ];
			while (j != i) {
				bool boundaryPoint = false;

				Point currPoint = contourLine[j];
				currentCandidate.length += magnitude(previousPoint, currPoint);
				currentCandidate.position.push_back(currPoint);
				if (angles[j] != dummyValue) {
					currentCandidate.curvature += angles[j];
				}
				else {
					boundaryPoint = true;
				}

				if (currentCandidate.length >= labelLength) {
					if (currentCandidate.curvature < candidatePosition.curvature) {
						candidatePosition = currentCandidate;
					}
					break;
				}
				
				if (boundaryPoint) {
					break;
				}

				previousPoint = currPoint;
				if (++j = contourLine.size()) {
					j = 0;
				}
			}
		}

		return candidatePosition.position;
	}

	// Computes a delta angle (curvature angle) for each point, which is constructed by 2 vectors: (current point - previous point), (next point - current point).
	static vector<vector<GLfloat>> computeCurvatureAngles(const vector<vector<Point>>& contour) {
		vector<vector<GLfloat>> angles;
		angles.reserve(contour.size());

		for (const vector<Point>& contourLine : contour) {
			vector<GLfloat> contourLineAngles;
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
	// Computes the magnitude of a vector given by 2 Point-s.
	static inline GLfloat magnitude(const Point& p1, const Point& p2) {
		Point vec{ p1.x - p2.x, p1.y - p2.y };
		return sqrt(vec.x * vec.x + vec.y * vec.y);
	}

	struct CandidatePosition{
		GLfloat curvature;
		GLfloat length;
		vector<Point> position;
	};

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