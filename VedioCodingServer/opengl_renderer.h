#ifndef OPENGL_RENDERER_H
#define OPENGL_RENDERER_H

#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "ply_loader.h"

extern GLFWwindow* window;

void initializeOpenGL();
void renderPoints(const std::vector<Point>& points);
void renderPoints(const std::vector<Point>& points, int frameIndex);
void cleanupOpenGL();
#endif // OPENGL_RENDERER_H
