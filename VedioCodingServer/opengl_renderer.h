#ifndef OPENGL_RENDERER_H
#define OPENGL_RENDERER_H

#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "ply_loader.h"

struct OpenGLContext {
	GLFWwindow* window;
	GLuint shaderProgram;
	GLuint VBO;
	GLuint VAO;
};

OpenGLContext initializeOpenGL();
//void renderPoints(const std::vector<Point>& points, int frameIndex);
void renderPoints(OpenGLContext& context, const std::vector<Point>& points, std::vector<unsigned char>& frameBuffer);
void cleanupOpenGL(OpenGLContext& context);
#endif // OPENGL_RENDERER_H
