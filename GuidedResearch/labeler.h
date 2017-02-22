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
	struct CandidatePosition {
		GLfloat curvature;
		GLfloat length;
		vector<Point> position;
	};

	// Searches for a point sequence, which comprises a broken line with minimal curvature and is at least labelLength long.
	static CandidatePosition findCandidatePositions(GLfloat labelLength, const vector<Point>& contourLine, const vector<GLfloat>& angles) {
		// TODO: for the 2 methods below, store length / angle, as opposed to just an agnle. Also an angle computation may need some changing (possibly: 180 - current angle)
		if (angles[angles.size() - 1] == dummyValue || angles[0] == dummyValue) {
			return findCandidatePositionsForOpenedContour(labelLength, contourLine, angles);
		}
		else {
			return findCandidatePositionsForClosedContour(labelLength, contourLine, angles);
		}
	}

	// Computes a delta angle (curvature angle) for each point, which is constructed by 2 vectors: (current point - previous point), (next point - current point). 
	// If given point is at boundary (one of the coordinates is -1 or 1), then dummyValue from point.h is stored for this point.
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
	// Returns the next looped index for i:[0, n - 1]. If i == (n - 1), returns 0.
	static inline int nextLoopedIndex(const int& i, const int& n) {
		return (i == n - 1) ? 0 : i + 1;
	}

	// Returns the previous looped index for i:[0, n - 1]. If i == 0, returns (n - 1).
	static inline int prevLoopedIndex(const int& i, const int& n) {
		return (i == 0) ? n - 1 : i - 1;
	}

	static CandidatePosition findCandidatePositionsForOpenedContour(GLfloat labelLength, const vector<Point>& contourLine, const vector<GLfloat>& angles) {
		CandidatePosition candidatePosition;

		// Check if contourLine is long enough to fit a label labelLength long.
		candidatePosition = contourLineToCandidatePosition(contourLine, angles);
		if (candidatePosition.length < labelLength) {
			return candidatePosition;
		}

		candidatePosition.curvature = numeric_limits<float>::max();
		candidatePosition.length = 0.f;

		int n = contourLine.size();
		for (int i = 0; i < n - 1; i++) {
			CandidatePosition currentCandidate;
			currentCandidate.position.clear();

			int nextI = i + 1;
			Point p1 = contourLine[i];
			Point p2 = contourLine[nextI];
			currentCandidate.position.push_back(p1);
			currentCandidate.position.push_back(p2);
			currentCandidate.length = magnitude(p1, p2);
			currentCandidate.curvature = 0;

			Point currPoint;
			Point previousPoint = p2;
			bool boundaryPoint;
			for (int j = nextI + 1; j < n; j++) {
				boundaryPoint = false;

				currPoint = contourLine[j];
				currentCandidate.length += magnitude(previousPoint, currPoint);
				currentCandidate.position.push_back(currPoint);
				// Check if we are at the NDC boundary and deal with angle accordingly
				if (angles[j - 1] == dummyValue) {
					boundaryPoint = true;
				} else {
					currentCandidate.curvature += angles[j - 1];
				}

				if (currentCandidate.length >= labelLength) {
					if (currentCandidate.curvature < candidatePosition.curvature) {
						candidatePosition = currentCandidate;
					}
					break;
				} else if (boundaryPoint) {
					break;
				}

				previousPoint = currPoint;
			}

		}

		return candidatePosition;
	}

	static CandidatePosition findCandidatePositionsForClosedContour(GLfloat labelLength, const vector<Point>& contourLine, const vector<GLfloat>& angles) {
		CandidatePosition candidatePosition;

		// Check if contourLine is long enough to fit a label labelLength long.
		candidatePosition = contourLineToCandidatePosition(contourLine, angles);
		if (candidatePosition.length < labelLength) {
			return candidatePosition;
		}

		candidatePosition.curvature = numeric_limits<float>::max();
		candidatePosition.length = 0.f;
		int n = contourLine.size();

		for (int i = 0; i < n; i++) {
			CandidatePosition currentCandidate;
			currentCandidate.position.clear();

			int nextI = nextLoopedIndex(i, n);
			Point p1 = contourLine[i];
			Point p2 = contourLine[nextI];
			currentCandidate.position.push_back(p1);
			currentCandidate.position.push_back(p2);
			currentCandidate.length = magnitude(p1, p2);
			currentCandidate.curvature = 0;

			Point currPoint;
			Point previousPoint = p2;
			int j = nextLoopedIndex(nextI, n);
			while (j != i) {
				currPoint = contourLine[j];
				currentCandidate.length += magnitude(previousPoint, currPoint);
				currentCandidate.position.push_back(currPoint);
				currentCandidate.curvature += angles[prevLoopedIndex(j, n)];

				if (currentCandidate.length >= labelLength) {
					if (currentCandidate.curvature < candidatePosition.curvature) {
						candidatePosition = currentCandidate;
					}
					break;
				}

				previousPoint = currPoint;
				j = nextLoopedIndex(j, n);
			}
		}

		return candidatePosition;
	}

	// TODO: can be made more efficient (i.e. do this when reconstruction a contourLine in contourer)
	// Used to make sure the contourLine is long enough to fit contour.
	static CandidatePosition contourLineToCandidatePosition(const vector<Point>& contourLine, const vector<GLfloat>& angles) {
		CandidatePosition candidatePosition;
		candidatePosition.position = contourLine;
		candidatePosition.length = 0.f;
		candidatePosition.curvature = 0.f;

		// Handle first
		if (angles[0] != dummyValue) {
			candidatePosition.curvature += angles[0];
		}
		candidatePosition.length += magnitude(contourLine[0], contourLine[1]);
		// Handle everything in the middle
		for (int i = 1; i < contourLine.size() - 1; i++) {
			candidatePosition.length += magnitude(contourLine[i], contourLine[i + 1]);
			candidatePosition.curvature += angles[i];
		}
		// Handle last
		if (angles[contourLine.size() - 1] != dummyValue) {
			candidatePosition.curvature += angles[contourLine.size() - 1];
		}

		return candidatePosition;
	}

	// Computes the magnitude of a vector given by 2 Point-s.
	static inline GLfloat magnitude(const Point& p1, const Point& p2) {
		Point vec{ p1.x - p2.x, p1.y - p2.y };
		return sqrt(vec.x * vec.x + vec.y * vec.y);
	}

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