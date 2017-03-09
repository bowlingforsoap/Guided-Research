#pragma once

#include <gl\glew.h>
#include <GLFW\glfw3.h>
#include <vector>

#include "line.h"

using namespace std;

// Traces contour for the given
template <size_t rows, size_t cols>
void traceContour(Line& currLine, int lineSegmentNumberInSquare, vector<Point>& contourLine, GLfloat(&feedback)[rows][cols], const int& i, const int& j, const Line& dummyLine, const GLint& fieldWidth);

// Tries to connect two compareLine to currLine. On sucess, returns true and alters currLine; on failure returns false.
inline bool connectedLines(Line& currLine, Line& compareLine, const Line& dummyLine, vector<Point>& contourLine);

// Returns all lines that belong to this contour.
template <size_t rows, size_t cols>
vector<vector<Point>> getContour(GLfloat(&feedback)[rows][cols], const GLint& fieldWidth, const GLint& fieldHeight)
{
	// Final result container
	vector<vector<Point>> contour;
	// Complete contour lines assembled from primitives (small line pieces) stored in feedback
	vector<Point> contourLine;
	Line dummyLine{ dummyValue, dummyValue, dummyValue, dummyValue };

	for (int i = 0; i < fieldWidth; i++)
	{
		for (int j = 0; j < cols; j = j + 2)
		{
			// Get contour line segments for current square
			Line currLines[2]{
				Line{
					Point{ feedback[i][j], feedback[i][j + 1] },
					Point{ feedback[i + fieldWidth][j], feedback[i + fieldWidth][j + 1] } },
				Line{
					Point{ feedback[i + fieldWidth * 2][j], feedback[i + fieldWidth * 2][j + 1] },
					Point{ feedback[i + fieldWidth * 3][j], feedback[i + fieldWidth * 3][j + 1] } }
			};

			for (int l = 0; l < 2; l++) {
				if (currLines[l] == dummyLine) {
					continue;
				}

				Line currLine = currLines[l];

				//				assert(currLine.begin.x != dummyValue && currLine.end.x != dummyValue && currLine.begin.y != dummyValue && currLine.end.y != dummyValue);

				contourLine.push_back(currLine.begin);
				contourLine.push_back(currLine.end);
				traceContour(currLine, l, contourLine, feedback, i, j, dummyLine, fieldWidth);

				if (contourLine.size() >= 2)
				{
					contour.push_back(contourLine);
					contourLine.clear(); // TODO: check if causes trouble
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
template <size_t rows, size_t cols>
void traceContour(Line& currLine, int lineSegmentNumberInSquare, vector<Point>& contourLine, GLfloat(&feedback)[rows][cols], const int& i, const int& j, const Line& dummyLine, const GLint& fieldWidth)
{
	// TODO: important! only delete the processed line, not the whole square!! otherwise an unprocessed line might get deleted.
	// Clear the data for the current square
	feedback[i + lineSegmentNumberInSquare * 2 * fieldWidth][j] = dummyLine.begin.x;
	feedback[i + lineSegmentNumberInSquare * 2 * fieldWidth][j + 1] = dummyLine.begin.y;
	feedback[i + (lineSegmentNumberInSquare * 2 + 1) * fieldWidth][j] = dummyLine.end.x;
	feedback[i + (lineSegmentNumberInSquare * 2 + 1) * fieldWidth][j + 1] = dummyLine.end.y;

	// Get line segments for squares around the current one
	//
	Line lineSegmentsAround[8];
	// (i + 1, j)
	if (i % (fieldWidth - 1) != 0 || i == 0) { // if we are not line-wise at the border of a layer
		lineSegmentsAround[0] = {
			Point{ feedback[i + 1][j], feedback[i + 1][j + 1] },
			Point{ feedback[i + 1 + fieldWidth][j], feedback[i + 1 + fieldWidth][j + 1] }
		};
		lineSegmentsAround[1] = {
			Point{ feedback[i + 1 + fieldWidth * 2][j], feedback[i + 1 + fieldWidth * 2][j + 1] },
			Point{ feedback[i + 1 + fieldWidth * 3][j], feedback[i + 1 + fieldWidth * 3][j + 1] }
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
			Point{ feedback[i - 1][j], feedback[i - 1][j + 1] },
			Point{ feedback[i - 1 + fieldWidth][j], feedback[i - 1 + fieldWidth][j + 1] }
		};
		lineSegmentsAround[3] = {
			Point{ feedback[i - 1 + fieldWidth * 2][j], feedback[i - 1 + fieldWidth * 2][j + 1] },
			Point{ feedback[i - 1 + fieldWidth * 3][j], feedback[i - 1 + fieldWidth * 3][j + 1] }
		};
	}
	else
	{
		lineSegmentsAround[2] = dummyLine;
		lineSegmentsAround[3] = dummyLine;
	}
	// (i, j + 2)
	if (j % (cols - 2) != 0 || j == 0) { // if we are not column-wise at the border of a layer
		lineSegmentsAround[4] = {
			Point{ feedback[i][j + 2], feedback[i][j + 1 + 2] },
			Point{ feedback[i + fieldWidth][j + 2], feedback[i + fieldWidth][j + 1 + 2] }
		};
		lineSegmentsAround[5] = {
			Point{ feedback[i + fieldWidth * 2][j + 2], feedback[i + fieldWidth * 2][j + 1 + 2] },
			Point{ feedback[i + fieldWidth * 3][j + 2], feedback[i + fieldWidth * 3][j + 1 + 2] }
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
			Point{ feedback[i][j - 2], feedback[i][j - 2 + 1] },
			Point{ feedback[i + fieldWidth][j - 2], feedback[i + fieldWidth][j - 2 + 1] }
		};
		lineSegmentsAround[7] = {
			Point{ feedback[i + fieldWidth * 2][j - 2], feedback[i + fieldWidth * 2][j - 2 + 1] },
			Point{ feedback[i + fieldWidth * 3][j - 2], feedback[i + fieldWidth * 3][j - 2 + 1] }
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
		connectedLinesFlag = connectedLines(currLine, compareLineSegment, dummyLine, contourLine);
		if (connectedLinesFlag)
		{
			// Go to the next square
			if (k == 0 || k == 1)
			{
				traceContour(currLine, k % 2, contourLine, feedback, i + 1, j, dummyLine, fieldWidth);
			}
			else if (k == 2 || k == 3)
			{
				traceContour(currLine, k % 2, contourLine, feedback, i - 1, j, dummyLine, fieldWidth);
			}
			else if (k == 4 || k == 5)
			{
				traceContour(currLine, k % 2, contourLine, feedback, i, j + 2, dummyLine, fieldWidth);
			}
			else if (k == 6 || k == 7)
			{
				traceContour(currLine, k % 2, contourLine, feedback, i, j - 2, dummyLine, fieldWidth);
			}

			// By not breaking the loop, we force algorithm to trace contour in a direction opposite to the one chosen before
		}
	}
}

// Definition.
inline bool connectedLines(Line& currLine, Line& compareLine, const Line& dummyLine, vector<Point>& contourLine)
{
	if (currLine.end == compareLine.begin && currLine.begin == compareLine.end) {
		cout << "break\n";
	}

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