#pragma once

#define _USE_MATH_DEFINES

#include <vector>
#include <gl\glew.h>
#include <math.h> /* atan2, abs */
#include <limits>

#include "point.h"

// Debug
//#include "renderer.h"
//#include <GLFW\glfw3.h>
//#include <GL\glew.h>

using namespace std;

// TODO: revise what to leave public and what to incapsulate.
class Labeler {
public:
	struct CandidatePosition {
		GLfloat curvature;
		GLfloat length;
		vector<Point> position;
	};


	// Points go clockwise in 'z' shape starting from top left point a.
	struct LabelCharacter {
		Point points[4];

		// Constructs a label character with center at (0, 0).
		LabelCharacter(const GLfloat& charWidth, const GLfloat& charHeight) {
			GLfloat halfWidth = charWidth / 2.f;
			GLfloat halfHeight = charHeight / 2.f;
			points[0] = { -halfWidth, halfHeight };
			points[1] = { halfWidth, halfHeight };
			points[2] = { -halfWidth, -halfHeight };
			points[3] = { halfWidth, -halfHeight };
		}
	};

	// Reresents one label.
	struct Label {
		GLfloat charWidth;
		GLfloat charHeight;
		vector<LabelCharacter> chars;

		// Constructs a label of numChars characters with the given charWidth and charHeight.
		Label(const int& numChars, const GLfloat& charWidth, const GLfloat& charHeight) {
			this->charWidth = charWidth;
			this->charHeight = charHeight;
			chars = vector<LabelCharacter>(numChars, LabelCharacter(charWidth, charHeight));
		}

		GLfloat getTotalWidth() {
			return charWidth * chars.size();
		}
	};

	// Converts LabelChar's points in a Label into a continious array of positions to be rendered. 
	static vector<Point> labelToPositionsArray(const Label& label) {
		vector<Point> positionsArray;
		positionsArray.reserve(label.chars.size() * 6); // 2 triangles per square == 6 points

		for (size_t i = 0; i < label.chars.size(); i++)
		{
			// First triangle
			positionsArray.push_back(label.chars[i].points[0]);
			positionsArray.push_back(label.chars[i].points[1]);
			positionsArray.push_back(label.chars[i].points[2]);
			// Second triangle
			positionsArray.push_back(label.chars[i].points[1]);
			positionsArray.push_back(label.chars[i].points[2]);
			positionsArray.push_back(label.chars[i].points[3]);
		}

		return positionsArray;
	}

	// Positions a provided label on the given line.
	// line - can be contour line or if the it's too short a line to place the label on.
	static void positionLabelOnLine(Label& label, const vector<Point>& line) {
		vector<GLfloat> distances = computeDistancesForLine(line);

		size_t start = 1;
		for (size_t i = 0; i < label.chars.size(); i++)
		{
			for (size_t j = start; j < distances.size(); j++)
			{
				GLfloat distanceToCharCenter = label.charWidth * i + label.charWidth / 2.f;
				if (distances[j] > distanceToCharCenter) {
					// Debug
					// Calculate transformation matrix for each point of a label's character
					glm::mat4 transf(1.f);
					// Translation
					GLfloat fraction = (distanceToCharCenter - distances[j - 1]) / (distances[j] - distances[j - 1]);
					Point prevPoint = line[j - 1];
					Point currPoint = line[j];
					glm::vec3 translation = glm::vec3(
						glm::mix(glm::vec2(prevPoint.x, prevPoint.y), glm::vec2(currPoint.x, currPoint.y), fraction),
						0.f
					);
					transf = glm::translate(transf, translation);
					// Rotation
					Point lineSegment = currPoint - prevPoint;
					GLfloat angle = atan2(lineSegment.y, lineSegment.x);// *180.f / M_PI;
					transf = glm::rotate(transf, angle, glm::vec3(0.f, 0.f, 1.f));
					// Apply transformation
					for (size_t k = 0; k < 4; k++)
					{
						glm::vec4 transfPoint = transf * glm::vec4(label.chars[i].points[k].x, label.chars[i].points[k].y, 0.f, 1.f);
						label.chars[i].points[k] = Point{ transfPoint.x, transfPoint.y };
					}

					// Debug char render
					/*glfwSwapBuffers(window);
					renderLabelCharacter(label.chars[i].points);
					glfwSwapBuffers(window);*/

					start = j;
					break;
				}
			}
		}
	}

	// Looks for a candidate position for a contourLine. Tries to find a point sequence, which comprises a broken line with minimal curvature and is at least labelLength long.
	static CandidatePosition findCandidatePositions(const GLfloat& labelLength, const GLfloat& labelHeight, const vector<Point>& contourLine, const vector<GLfloat>& angles) {
		if (angles[angles.size() - 1] == dummyValue || angles[0] == dummyValue) {
			return findCandidatePositionsForOpenedContour(labelLength, labelHeight, contourLine, angles);
		}
		else {
			return findCandidatePositionsForClosedContour(labelLength, labelHeight, contourLine, angles);
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

			if (contourLine.size() > 2) // case for 1-segment contourLine
			{
				for (int i = 1; i < contourLine.size() - 1; i++) 
				{
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

			}
			else 
			{
				contourLineAngles.push_back(dummyValue);
				contourLineAngles.push_back(dummyValue);
			}

			angles.push_back(contourLineAngles);

		}

		return angles;
	}

private:
	// Computes distances from the beginning of the line to each of its points.
	static vector<GLfloat> computeDistancesForLine(const vector<Point>& line) {
		vector<GLfloat> distances;
		distances.push_back(0.f);

		GLfloat prevDistance = 0;
		Point prevPoint = line[0];
		for (size_t i = 1; i < line.size(); i++)
		{
			GLfloat currDistance = magnitude(prevPoint, line[i]);
			distances.push_back(currDistance + prevDistance);

			prevPoint = line[i];
			prevDistance += currDistance;
		}

		return distances;
	}

	// Returns the next looped index for i:[0, n - 1]. If i == (n - 1), returns 0.
	static inline int nextLoopedIndex(const int& i, const int& n) {
		return (i == n - 1) ? 0 : i + 1;
	}

	// Returns the previous looped index for i:[0, n - 1]. If i == 0, returns (n - 1).
	static inline int prevLoopedIndex(const int& i, const int& n) {
		return (i == 0) ? n - 1 : i - 1;
	}

	static CandidatePosition candidatePositionForShortContourLine(/*const CandidatePosition& candidatePosition,*/ const GLfloat& labelLength, const GLfloat& labelHeight, const vector<Point>& contourLine) {
		CandidatePosition result;
		Point start;

		Point labelPoint = contourLine[0];
		// Decide top or bottom
		if (labelPoint.y >= 0.f) {
			start.y = labelPoint.y - labelHeight / 2.f;
		}
		else {
			start.y = labelPoint.y + labelHeight / 2.f;
		}
		// Decide left or right
		if (labelPoint.x >= 0.f) {
			start.x = labelPoint.x - labelLength;
		}
		else {
			start.x = labelPoint.x;
		}

		result.curvature = 0.f;
		result.length = labelLength;
		result.position.push_back(start);
		result.position.push_back(Point{ start.x + labelLength, start.y });

		return result;
	}

	static CandidatePosition findCandidatePositionsForOpenedContour(const GLfloat& labelLength, const GLfloat& labelHeight, const vector<Point>& contourLine, const vector<GLfloat>& angles) {
		CandidatePosition candidatePosition;

		// Check if contourLine is long enough to fit a label labelLength long.
		candidatePosition = contourLineToCandidatePosition(contourLine, angles);
		if (candidatePosition.length < labelLength) {
			candidatePosition = candidatePositionForShortContourLine(labelLength, labelHeight, contourLine);
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
				}
				else {
					currentCandidate.curvature += angles[j - 1];
				}

				if (currentCandidate.length >= labelLength) {
					if (currentCandidate.curvature < candidatePosition.curvature) {
						candidatePosition = currentCandidate;
					}
					break;
				}
				else if (boundaryPoint) {
					break;
				}

				previousPoint = currPoint;
			}

		}

		return candidatePosition;
	}

	static CandidatePosition findCandidatePositionsForClosedContour(const GLfloat& labelLength, const GLfloat& labelHeight, const vector<Point>& contourLine, const vector<GLfloat>& angles) {
		CandidatePosition candidatePosition;

		// Check if contourLine is long enough to fit a label labelLength long.
		candidatePosition = contourLineToCandidatePosition(contourLine, angles);
		if (candidatePosition.length < labelLength) {
			candidatePosition = candidatePositionForShortContourLine(labelLength, labelHeight, contourLine);
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
	// Makes sure the contourLine is long enough to fit contour.
	static CandidatePosition contourLineToCandidatePosition(const vector<Point>& contourLine, const vector<GLfloat>& angles) {
		CandidatePosition candidatePosition;
		candidatePosition.position = contourLine; // is this necessary?
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