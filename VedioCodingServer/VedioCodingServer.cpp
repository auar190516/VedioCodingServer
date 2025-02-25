#include <iostream>
#include <vector>
#include "ply_loader.h"
#include "opengl_renderer.h"

// 主函数
int main() {
	// 加载PLY文件，替换为你实际的文件路径
	std::string filename = "longdress_vox10_1053.ply";
	std::vector<Point> points = loadPLY(filename);

	// 如果加载失败或者没有点数据，退出
	if (points.empty()) {
		std::cerr << "Failed to load PLY file or no points found!" << std::endl;
		return -1;
	}
	std::cout << "Loaded " << points.size() << " points from the PLY file." << std::endl;
	// 初始化 OpenGL 环境
	initializeOpenGL();

	// 渲染循环
	while (!glfwWindowShouldClose( window )) {
		// 处理用户输入（如窗口关闭、键盘等）
		glfwPollEvents();

		// 渲染点云
		renderPoints(points);

		// 交换缓冲区，显示当前帧
		glfwSwapBuffers(window);
	}

	// 清理资源
	cleanupOpenGL();

	return 0;
}
