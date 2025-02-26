
#include <iostream>
#include <fstream>
#include <iomanip> // ���ڸ�ʽ���ļ���
#include <sstream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "opengl_renderer.h"

GLFWwindow* window = nullptr;
// ��ɫ������
const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPos;  // �������
layout(location = 1) in vec3 aColor; // �����ɫ

out vec3 fragColor;  // ����ɫ���ݸ�Ƭ����ɫ��

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    fragColor = aColor;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
in vec3 fragColor;  // �Ӷ�����ɫ����������ɫ

out vec4 FragColor;

void main() {
    FragColor = vec4(fragColor, 1.0);  // ���õ����ɫ
}
)";

GLuint shaderProgram;

GLuint VBO, VAO;

// ��ȡ֡���������ݲ�����Ϊ PPM ͼ��
void saveFrame(int frameIndex, int width, int height) {
	std::vector<unsigned char> pixels(width * height * 3);

	// ��ȡ OpenGL ��֡���������ݣ�ע�⣺OpenGL �����½ǿ�ʼ�洢ͼ��
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

	// �����ļ������� "frame_0001.ppm"
	std::ostringstream filename;
	filename << "frame_" << std::setw(4) << std::setfill('0') << frameIndex << ".ppm";

	// ���� PPM �ļ�
	std::ofstream file(filename.str(), std::ios::binary);
	if (!file) {
		std::cerr << "Failed to save frame: " << filename.str() << std::endl;
		return;
	}

	// д�� PPM ͷ����P6 ��ʽ��
	file << "P6\n" << width << " " << height << "\n255\n";

	// OpenGL �洢��ͼ�������·�ת�ģ���Ҫ����������
	for (int y = height - 1; y >= 0; --y) {
		file.write(reinterpret_cast<char*>(pixels.data() + y * width * 3), width * 3);
	}

	file.close();
	std::cout << "Saved frame: " << filename.str() << std::endl;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	// �����ӿ�
	glViewport(0, 0, width, height);

	// ���¼���ͶӰ���󣬸����µĿ�߱�
	glm::mat4 projection = glm::perspective(glm::radians(60.0f),
		static_cast<float>(width) / static_cast<float>(height), 1.0f, 1000.0f);

	// ����ͶӰ����
	GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
}
void initializeOpenGL() {
	if (!glfwInit()) {
		std::cerr << "GLFW Initialization Failed!" << std::endl;
		exit(EXIT_FAILURE);
	}

	window = glfwCreateWindow(960, 1280, "Point Cloud Renderer", NULL, NULL);
	if (!window) {
		glfwTerminate();
		std::cerr << "Failed to create GLFW window!" << std::endl;
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize OpenGL loader!" << std::endl;
		exit(EXIT_FAILURE);
	}

	// ����OpenGL�ӿ�
	glViewport(0, 0, 960, 1280);
	//glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	// ������ɫ������
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
	glCompileShader(vertexShader);

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
	glCompileShader(fragmentShader);

	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	glUseProgram(shaderProgram);

	// ����VAO��VBO
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
}
void renderPoints(const std::vector<Point>& points, int frameIndex) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(Point), points.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(Point), (void*)offsetof(Point, r));
	glEnableVertexAttribArray(1);

	// ����ģ�͡���ͼ��ͶӰ����
	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 100.0f, -250.0f), glm::vec3(0.0f, 100.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 projection = glm::perspective(glm::radians(60.0f), 1.0f, 1.0f, 1000.0f);

	GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
	GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
	GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	glDrawArrays(GL_POINTS, 0, points.size());

	glBindVertexArray(0);

	// ���浱ǰ֡
	saveFrame(frameIndex, 960, 1280); // ����Դ��� OpenGL ���ڵ�ʵ�ʿ��
}
void renderPoints(const std::vector<Point>& points) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(Point), points.data(), GL_STATIC_DRAW);

	// ���ö�������ָ��
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(Point), (void*)offsetof(Point, r));
	glEnableVertexAttribArray(1);

	// ���þ���ģ�͡���ͼ��ͶӰ��
	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 100.0f, -250.0f), glm::vec3(0.0f, 100.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 projection = glm::perspective(glm::radians(60.0f), 1.0f, 1.0f, 1000.0f);

	GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
	GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
	GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	glDrawArrays(GL_POINTS, 0, points.size());

	glBindVertexArray(0);
}


void cleanupOpenGL() {
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glfwDestroyWindow(window);
	glfwTerminate();
}
