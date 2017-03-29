#pragma once

#include <gl\glew.h>
#include <GLFW\glfw3.h>
#include <vector>

#include "line.h"
#include "renderer.h"

using namespace std;

// Convert 2D index to 1D index.
#define to1D(i,j) ((i) * fieldHeight * 2 + j)

class Contourer {
private:
	GLint fieldWidth;
	GLint fieldHeight;

	const Line dummyLine{dummyValue, dummyValue, dummyValue, dummyValue};

	/*inline size_t to1D(const int& i, const int& j) const {
		return i * fieldHeight + j;
	}*/

public:
	Contourer(GLint fieldWidth, GLint fieldHeight) : fieldWidth(fieldWidth), fieldHeight(fieldHeight) {

	}

	// Returns all lines that belong to this contour.
	vector<vector<Point>> getContour(vector<GLfloat>& feedback) const
	{
		// Final result container
		vector<vector<Point>> contour;
		// Complete contour lines assembled from primitives (small line pieces) stored in feedback
		vector<Point> contourLine;

		for (int i = 0; i < fieldWidth; i++)
		{
			for (int j = 0; j < fieldHeight * 2; j = j + 2)
			{
				// Get contour line segments for current square
				Line currLines[2]{
					Line{
						Point{
							feedback[to1D(i, j)],
							feedback[to1D(i, j + 1)] },
						Point{
							feedback[to1D(i + fieldWidth, j)],
							feedback[to1D(i + fieldWidth, j + 1)] } },
					Line{
						Point{
							feedback[to1D(i + fieldWidth * 2, j)],
							feedback[to1D(i + fieldWidth * 2, j + 1)] },
						Point{
							feedback[to1D(i + fieldWidth * 3, j)],
							feedback[to1D(i + fieldWidth * 3, j + 1)] } }
				};

				for (int l = 0; l < 2; l++) {
					if (currLines[l] == dummyLine) {
						continue;
					}

					Line currLine = currLines[l];

					//				assert(currLine.begin.x != dummyValue && currLine.end.x != dummyValue && currLine.begin.y != dummyValue && currLine.end.y != dummyValue);

					contourLine.push_back(currLine.begin);
					contourLine.push_back(currLine.end);
					traceContour(currLine, l, contourLine, feedback, i, j);

					if (contourLine.size() >= 2)
					{
						contour.push_back(contourLine);
						contourLine.clear();
					}
				}
			}
		}

		// The one before last point in closed contour lines dublicates the first one, which is to be expected, as a line closes on itself. However, we don't need to store it. Additionally, due to recursive nature of the algorithm and the way lines around the current one are stored, linesAround for the very first point do not reflect the changes made to feedback array, from which they were initially taken. Therefore, the other end of the line (which would be followed, had the algorithm chose the opposite way to trace this paticular contour) is stored twice: as the second element before last and as the last one. 
		// Conclusion: we have to delete the last two points, as they are redundant and harmful to further labeling.
		for (vector<Point>& contourLine : contour) {
			if (contourLine.size() > 2)
				if (contourLine[contourLine.size() - 1] == contourLine[contourLine.size() - 3]) {
					contourLine.pop_back();
					contourLine.pop_back();
				}
		}
		return contour;
	}

	// Traces contour starting from currLine and fills-up the contourLine vector with found points.
	//	lineSegmentNumberInSquare - 1,2 depending on wheather it's the 1st or the 2nd line stored in the square.
	void traceContour(Line& currLine, const int& lineSegmentNumberInSquare, vector<Point>& contourLine, vector<GLfloat>& feedback, const int& i, const int& j) const
	{
		// Clear the data for the current square
		feedback[to1D(i + lineSegmentNumberInSquare * 2 * fieldWidth, j)] = dummyLine.begin.x;
		feedback[to1D(i + lineSegmentNumberInSquare * 2 * fieldWidth, j + 1)] = dummyLine.begin.y;
		feedback[to1D(i + (lineSegmentNumberInSquare * 2 + 1) * fieldWidth, j)] = dummyLine.end.x;
		feedback[to1D(i + (lineSegmentNumberInSquare * 2 + 1) * fieldWidth, j + 1)] = dummyLine.end.y;

		// Get line segments for squares around the current one
		//
		Line lineSegmentsAround[8];
		// (i + 1, j)
		if (i % (fieldWidth - 1) != 0 || i == 0) { // if we are not line-wise at the border of a layer
			lineSegmentsAround[0] = {
				Point{ feedback[to1D(i + 1, j)], feedback[to1D(i + 1, j + 1)] },
				Point{ feedback[to1D(i + 1 + fieldWidth, j)], feedback[to1D(i + 1 + fieldWidth, j + 1)] }
			};
			lineSegmentsAround[1] = {
				Point{ feedback[to1D(i + 1 + fieldWidth * 2, j)], feedback[to1D(i + 1 + fieldWidth * 2, j + 1)] },
				Point{ feedback[to1D(i + 1 + fieldWidth * 3, j)], feedback[to1D(i + 1 + fieldWidth * 3, j + 1)] }
			};
		}
		else
		{
			lineSegmentsAround[0] = dummyLine;
			lineSegmentsAround[1] = dummyLine;
		}
		// (i - 1, j)
		if (i % fieldWidth != 0) { // if we are not line-wise at the border of a layer on the other side
			lineSegmentsAround[2] = {
				Point{ feedback[to1D(i - 1, j)], feedback[to1D(i - 1, j + 1)] },
				Point{ feedback[to1D(i - 1 + fieldWidth, j)], feedback[to1D(i - 1 + fieldWidth, j + 1)] }
			};
			lineSegmentsAround[3] = {
				Point{ feedback[to1D(i - 1 + fieldWidth * 2, j)], feedback[to1D(i - 1 + fieldWidth * 2, j + 1)] },
				Point{ feedback[to1D(i - 1 + fieldWidth * 3, j)], feedback[to1D(i - 1 + fieldWidth * 3, j + 1)] }
			};
		}
		else
		{
			lineSegmentsAround[2] = dummyLine;
			lineSegmentsAround[3] = dummyLine;
		}
		// (i, j + 2)
		if (j % (fieldHeight * 2 - 2) != 0 || j == 0) { // if we are not column-wise at the border of a layer
			lineSegmentsAround[4] = {
				Point{ feedback[to1D(i, j + 2)], feedback[to1D(i, j + 1 + 2)] },
				Point{ feedback[to1D(i + fieldWidth, j + 2)], feedback[to1D(i + fieldWidth, j + 1 + 2)] }
			};
			lineSegmentsAround[5] = {
				Point{ feedback[to1D(i + fieldWidth * 2, j + 2)], feedback[to1D(i + fieldWidth * 2, j + 1 + 2)] },
				Point{ feedback[to1D(i + fieldWidth * 3, j + 2)], feedback[to1D(i + fieldWidth * 3, j + 1 + 2)] }
			};
		}
		else
		{
			lineSegmentsAround[4] = dummyLine;
			lineSegmentsAround[5] = dummyLine;
		}
		// (i, j - 2)
		if (j != 0) { // if we are not column-wise at the border of a layer from the other side
			lineSegmentsAround[6] = {
				Point{ feedback[to1D(i, j - 2)], feedback[to1D(i, j - 2 + 1)] },
				Point{ feedback[to1D(i + fieldWidth, j - 2)], feedback[to1D(i + fieldWidth, j - 2 + 1)] }
			};
			lineSegmentsAround[7] = {
				Point{ feedback[to1D(i + fieldWidth * 2, j - 2)], feedback[to1D(i + fieldWidth * 2, j - 2 + 1)] },
				Point{ feedback[to1D(i + fieldWidth * 3, j - 2)], feedback[to1D(i + fieldWidth * 3, j - 2 + 1)] }
			};
		}
		else
		{
			lineSegmentsAround[6] = dummyLine;
			lineSegmentsAround[7] = dummyLine;
		}

		// Compare line with linesAround
		// Trace contour for this line
		//
		bool connectedLinesFlag = false;
		for (int k = 0; k < 8; k++)
		{
			Line compareLineSegment = lineSegmentsAround[k];
			if (compareLineSegment == dummyLine)
			{
				continue;
			}

			//		assert(compareLineSegment.begin.x != dummyLine.begin.x && compareLineSegment.end.x != dummyLine.begin.x && compareLineSegment.begin.y != dummyLine.begin.x && compareLineSegment.end.y != dummyLine.begin.x);
			connectedLinesFlag = connectedLines(currLine, compareLineSegment, contourLine);
			if (connectedLinesFlag)
			{
				// Go to the next square
				if (k == 0 || k == 1)
				{
					traceContour(currLine, k % 2, contourLine, feedback, i + 1, j);
				}
				else if (k == 2 || k == 3)
				{
					traceContour(currLine, k % 2, contourLine, feedback, i - 1, j);
				}
				else if (k == 4 || k == 5)
				{
					traceContour(currLine, k % 2, contourLine, feedback, i, j + 2);
				}
				else if (k == 6 || k == 7)
				{
					traceContour(currLine, k % 2, contourLine, feedback, i, j - 2);
				}

				// ~? By not breaking the loop, we force algorithm to trace contour in a direction opposite to the one chosen before
			}
		}
	}

	// Definition.
	inline bool connectedLines(Line& currLine, Line& compareLine, vector<Point>& contourLine) const
	{
		if (currLine.end == compareLine.begin)
		{
			contourLine.push_back(compareLine.end);
			currLine.end = compareLine.end;

			return true;
		}
		else if (currLine.end == compareLine.end)
		{
			contourLine.push_back(compareLine.begin);
			currLine.end = compareLine.begin;

			return true;
		}
		else if (currLine.begin == compareLine.begin)
		{
			contourLine.insert(contourLine.begin(), compareLine.end);
			currLine.begin = compareLine.end;

			return true;
		}
		else if (currLine.begin == compareLine.end)
		{
			contourLine.insert(contourLine.begin(), compareLine.begin);
			currLine.begin = compareLine.begin;

			return true;
		}

		return false;
	}
};