#include <util/GLFWinitializer.h>
#include <util/Shader.h>
#include <SOIL.h>

// Generates scalar field (which can be placed in a texture) and texture coordinates for it
/*
scalarField:[[minX, maxX], [minY, maxY]]
fieldCoords:[[0, width - 2], [0, height - 2]]
*/
void generateScalarField(GLfloat* &scalarField, GLint width, GLint height, GLfloat minX, GLfloat minY, GLfloat maxX, GLfloat maxY, GLint* &fieldCoords, GLint &fieldCoordsSize) {
	scalarField = new GLfloat[width * height];
	float xDelta = abs(maxX - minX) / width;
	float yDelta = abs(maxY - minY) / height;

	// Fill scalar field with f:sin(x)*cos(x)
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			scalarField[i * height + j] = sin(minX + xDelta * i) * cos(minY + yDelta * j);
		}
	}

	// Fill fieldCoords: coordinates of the upper left corner of each square in context of marching squares
	fieldCoordsSize = (width - 1) * (height - 1) * 2;
	fieldCoords = new GLint[fieldCoordsSize];
	int i = 0;
	while (i < (width - 1) * (height - 1)) {
		fieldCoords[i * 2] = i / (height - 1);
		fieldCoords[i * 2 + 1] = i % (height - 1);
		i = i++;
	}
}

int main() {
	GLint width = 1600, height = 1000;
	GLFWwindow* window = glfwInitialize(width, height, "Guided Research", 4, 2, false);
	glewInit();
	glViewport(0, 0, width, height);

	// Setup shaders
	Shader shader("vert.glsl", "frag.glsl", "geometry.glsl");
	// Check some OpenGL capabilities
	GLint value;
	glGetIntegerv(GL_MAX_GEOMETRY_IMAGE_UNIFORMS, &value);
	cout << "GL_MAX_GEOMETRY_IMAGE_UNIFORMS: " << value << endl;
	glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, &value);
	cout << "GL_MAX_COMPUTE_SHARED_MEMORY_SIZE: " << value << endl;

	// Scalar Field setup
	GLint fieldWidth = 100, fieldHeight = 100;
	GLfloat* scalarField = 0;
	GLint* fieldCoords = 0;
	GLint fieldCoordsSize;
	generateScalarField(scalarField, fieldWidth, fieldHeight, -3, -2, 3, 2, fieldCoords, fieldCoordsSize);

	// Check scalar field coords
	/*for (int i = fieldCoordsSize - 1; i > fieldCoordsSize - 20; i--) {
		cout << scalarField[i] << ", ";
	}
	cout << endl;
	for (int i = 0; i < 20; i++) {
		cout << scalarField[i] << ", ";
	}*/

	// Store scalar field into a texture
	GLuint scalarFieldTex;
	glGenTextures(1, &scalarFieldTex);
	glBindTexture(GL_TEXTURE_2D, scalarFieldTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, fieldWidth, fieldHeight, 0, GL_RED, GL_FLOAT, scalarField);
	glBindImageTexture(0, scalarFieldTex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
	glBindTexture(GL_TEXTURE_2D, 0);
	//cout << glGetError() << endl;
	delete[] scalarField;
	// Set uniforms
	shader.Use();
	glUniform1i(glGetUniformLocation(shader.Program, "scalarField"), 0); // image unit 0
	glUniform1f(glGetUniformLocation(shader.Program, "isoValue"), .5f);

	// Points to render
	GLfloat points[] {
		0.f, .0f, 0.f
	};

	// Setup attributes
	GLuint VAO, VBO;
	glGenBuffers(1, &VBO);
	glGenVertexArrays(1, &VAO);
	// Bind data
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, fieldCoordsSize * sizeof(GLint), fieldCoords, GL_STATIC_DRAW);
	// Setup vertex array
	glBindVertexArray(VAO);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_INT, GL_FALSE, 2 * sizeof(GLint), 0);
	glBindVertexArray(0);


	glClearColor(0.1f, 0.1f, 0.1f, 1.f);
	// Gameloop
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		glClear(GL_COLOR_BUFFER_BIT);

		shader.Use();
		//glBindTexture(GL_TEXTURE_2D, scalarFieldTex);
		glBindVertexArray(VAO);
		glDrawArrays(GL_POINTS, 0, fieldCoordsSize);
		glBindVertexArray(0);

		glfwSwapBuffers(window);
	}

	glfwTerminate();
	return 666;
}