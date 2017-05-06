#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#define GLEW_STATIC
#include <GL/glew.h>

#define GL_ERROR_CHECK cout << "Error check: 0x" << std::hex << glGetError() << std::dec << ", Line: " << __LINE__ << ", File: " << __FILE__ << "." << endl;

using namespace std;

class Shader {
public:
	// The program ID
	GLuint Program;

	// Constructor reads and builds the shader
	Shader(const GLchar* vertexPath, const GLchar* fragmentPath, const GLchar* geometryPath) {
		// 1. Retrieve the vertex/fragment source code from filePath
		std::string vertexCode;
		std::string fragmentCode;
		std::string geometryCode;

		// ensures ifstream objects can throw exceptions:
		/**shaderFile.exceptions(std::ifstream::badbit);
		fShaderFile.exceptions(std::ifstream::badbit);
		try
		{
		// Open files
		shaderFile.open(vertexPath);
		fShaderFile.open(fragmentPath);
		std::stringstream vShaderStream, fShaderStream;
		// Read file's buffer contents into streams
		vShaderStream << shaderFile.rdbuf();
		fShaderStream << fShaderFile.rdbuf();
		// close file handlers
		shaderFile.close();
		fShaderFile.close();
		// Convert stream into GLchar array
		vertexCode = vShaderStream.str();
		fragmentCode = fShaderStream.str();
		}
		catch (std::ifstream::failure e)
		{
		std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
		}**/

		vertexCode = getShaderCode(vertexPath);
		fragmentCode = getShaderCode(fragmentPath);
		geometryCode = getShaderCode(geometryPath);

		const GLchar* vShaderCode = vertexCode.c_str();


		// 2. Compile shaders
		GLuint vertex = 0, fragment = 0, geometry = 0;
		vertex = createAndCompileShader(GL_VERTEX_SHADER, vShaderCode);

		if (fragmentCode != "") {
			const GLchar* fShaderCode = fragmentCode.c_str();
			fragment = createAndCompileShader(GL_FRAGMENT_SHADER, fShaderCode);
		}
		// Geometry shader
		if (geometryCode != "") {
			const GLchar* gShaderCode = geometryCode.c_str();
			geometry = createAndCompileShader(GL_GEOMETRY_SHADER, gShaderCode);
		}

		// Shader Program
		GLint success;
		GLchar infoLog[512];
		this->Program = glCreateProgram();
		glAttachShader(this->Program, vertex);
		if (fragment != 0) {
			glAttachShader(this->Program, fragment);
		}
		if (geometry != 0) {
			glAttachShader(this->Program, geometry);
		}
		glLinkProgram(this->Program);
		// Print linking errors if any
		glGetProgramiv(this->Program, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(this->Program, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
		}


		// Delete the shaders as they're linked into our program now and no longer necessery
		glDeleteShader(vertex);
		if (fragment != 0) {
			glDeleteShader(fragment);
		}
		if (geometry != 0) {
			glDeleteShader(geometry);
		}
	}

	// Shader code retreiving function
	static std::string getShaderCode(const GLchar* shaderPath) {
		std::string shaderCode;
		std::ifstream shaderFile;

		// Open input file stream and chek it
		shaderFile.open(shaderPath);
		if (!shaderFile.good()) {
			cerr << "ERROR OPENING SHADER FILE: \"" << shaderPath << "\"" << endl;
			//system("pause");
			//exit(1);
			return "";
		}

		// Read shader code
		string line;
		while (!shaderFile.eof()) {
			getline(shaderFile, line);
			shaderCode += line;
			shaderCode += "\n";
		}
		shaderFile.close();

		return shaderCode;
	}

	// Use Program
	void Use() {
		glUseProgram(this->Program);
	}

	static void inline checkShaderCompilationStatus(const GLuint &shader, const GLchar* &shaderCode) {
		GLint success;
		GLchar infoLog[512];

		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(shader, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::COMPILATION_FAILED:\n" << shaderCode << "\n" << infoLog << std::endl;
		};
	}

	static void inline checkProgramLinkingStatus(const GLuint &program) {
		GLint success;
		GLchar infoLog[512];

		glGetProgramiv(program, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(program, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
		}
	}

private:
	GLuint createAndCompileShader(GLenum shaderType, const GLchar* shaderCode) {
		GLuint shader;
		GLint success;
		GLchar infoLog[512];

		shader = glCreateShader(shaderType);
		glShaderSource(shader, 1, &shaderCode, NULL);
		glCompileShader(shader);
		// Print compile errors if any
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(shader, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::COMPILATION_FAILED:\n" << shaderCode << "\n" << infoLog << std::endl;
		};

		return shader;
	}

};