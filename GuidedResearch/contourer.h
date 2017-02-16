#pragma once

#include <gl\glew.h>
#include <GLFW\glfw3.h>
#include <vector>

using namespace std;

#define GL_ERROR_CHECK cout << "Error check: " << glGetError() << endl;

// Used to deal with floating point precision issues.
GLfloat epsilon = .000001f;

struct Point {
	GLfloat x;
	GLfloat y;

	inline bool operator==(const Point& rhs) {
		if (x == rhs.x && y == rhs.y) {
			return true;
		}
		if (Point{ abs(x - rhs.x), abs(y - rhs.y) } < Point{ epsilon, epsilon }) {
			return true;
		}
		else {
			return false;
		}
	}

	inline Point operator-(const Point& rhs) {
		return { x - rhs.x, y - rhs.y };
	}

	inline bool operator<(const Point& rhs) {
		if (x < rhs.x && y < rhs.y) {
			return true;
		}

		return false;
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

	// For each separate part of an iso-contour
	while (feedbackVector.size() >= 4) {
		contourLine.clear();

		// Get last 4 elements
		int lastEl = feedbackVector.size() - 1;
		Line l1 = { Point{feedbackVector[lastEl - 3], feedbackVector[lastEl - 2]}, Point{ feedbackVector[lastEl - 1], feedbackVector[lastEl]} };

		feedbackVector._Pop_back_n(4);

		contourLine.push_back(l1.begin);
		contourLine.push_back(l1.end);

		// Indicates that the line did not loop on itself and has to be traced to the other direction from the start
		bool traceBack = false;
		int i = 0;
		
		if (!traceBack) {
			// Try to append lines to the back of l1
			while (i <= feedbackVector.size() - 4) {
				//while (true) {
					// Like this because size changes dynamically and while seems to cache .size() result
				if (feedbackVector.size() == 0 || l1.begin == l1.end) {
					break;
				}

				Line l2 = { Point{feedbackVector[i], feedbackVector[i + 1]}, Point{ feedbackVector[i + 2], feedbackVector[i + 3]} };

				// Prevents dublicating the first point
				if (l1.end == l2.begin && l1.begin == l2.end) {
					feedbackVector.erase(feedbackVector.begin() + i, feedbackVector.begin() + i + 4);
					break;
				}

				// If l2 continues l1
				if (l1.end == l2.begin) {
					// Add it l2 to the back of contourLine
					contourLine.push_back(l2.end);
					l1.end = l2.end;
					// Remove l2 from the feedbackVector
					feedbackVector.erase(feedbackVector.begin() + i, feedbackVector.begin() + i + 4);
					// Start comparison from the begining again
					i = 0;
				} else if (l1.end == l2.end) {
					// Add it l2 to the back of contourLine reversed
					contourLine.push_back(l2.begin);
					l1.end = l2.begin;
					// Remove l2 from the feedbackVector
					feedbackVector.erase(feedbackVector.begin() + i, feedbackVector.begin() + i + 4);
					// Start comparison from the begining again
					i = 0;
				} else if (abs(abs(l1.end.x) - 1.0f) < epsilon || abs(abs(l1.end.y) - 1.f) < epsilon) {
					// If we hit the edge of the screen
					traceBack = true;
					break;
				} else {
					// continue search
					i += 4;
					continue;
				}
			}
		}

		if (traceBack) {
			// Try to append lines to the front of l1
			i = 0;
			while (i <= feedbackVector.size() - 4) {
				//while (true) {
				// Like this because size changes dynamically and while seems to cache .size() result
				if (feedbackVector.size() == 0) {
					break;
				}

				Line l2 = { Point{ feedbackVector[i], feedbackVector[i + 1] }, Point{ feedbackVector[i + 2], feedbackVector[i + 3] } };

				// Prevents dublicating the first point
				if (l1.end == l2.begin && l1.begin == l2.end) {
					feedbackVector.erase(feedbackVector.begin() + i, feedbackVector.begin() + i + 4);
					break;
				}

				// If l1 continues l2
				if (l1.begin == l2.end) {
					contourLine.insert(contourLine.begin(), l2.begin);
					l1.begin = l2.begin;
					// Remove l2 from the feedbackVector
					feedbackVector.erase(feedbackVector.begin() + i, feedbackVector.begin() + i + 4);
					// Start comparison from the begining again
					i = 0;
				} else if (l1.begin == l2.begin) {
					contourLine.insert(contourLine.begin(), l2.end);
					l1.begin = l2.end;
					// Remove l2 from the feedbackVector
					feedbackVector.erase(feedbackVector.begin() + i, feedbackVector.begin() + i + 4);
					// Start comparison from the begining again
					i = 0;
				} else if (abs(abs(l1.begin.x) - 1.f) < epsilon || abs(abs(l1.begin.y) - 1.f) < epsilon) { // TODO: move up
					// If we hit the edge of the screen again
					break;
				} else {
					// continue search
					i += 4;
					continue;
				}
			}
		}

		contour.push_back(contourLine);
	}

	return contour;
}


void renderContour(GLFWwindow& window, vector<vector<Point>>& contour) {
	Shader shader("shaders/reconstructedcontour/vert.glsl", "shaders/reconstructedcontour/frag.glsl", "");

	GLuint VBO, VAO;
	glGenBuffers(1, &VBO);
	glGenVertexArrays(1, &VAO);

	// TODO: unhardcode or remove

	const int colorsSize = 2;
	GLfloat colors[colorsSize][3]{
		.6f, .1f, .23f,
		.1f, .6f, .23f
	};

	for (int i = 0; i < contour.size(); i++) {
		cout << "contour[" << i << "].size(): " << contour[i].size() << endl;

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, contour[i].size() * 2 * sizeof(GLfloat), &contour[i], GL_STATIC_DRAW);

		glBindVertexArray(VAO);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);

		glLineWidth(5.f);

		shader.Use();
		//glUniform3fv(glGetUniformLocation(shader.Program, "u_Color"), 3, colors[ 0]);
		glDrawArrays(GL_LINE_STRIP, 0, contour[i].size());
		glBindVertexArray(0);
	}
	glfwSwapBuffers(&window);
	// Clean-up
	glDeleteBuffers(1, &VBO);
	glDeleteVertexArrays(1, &VAO);
}

