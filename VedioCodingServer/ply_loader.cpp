#include "ply_loader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

std::vector<Point> loadPLY(const std::string& filename) {
	std::ifstream file(filename);
	std::string line;
	std::vector<Point> points;

	if (!file.is_open()) {
		std::cerr << "Failed to open file: " << filename << std::endl;
		return points;
	}

	bool inHeader = true;
	int vertexCount = 0;
	float scale = 1.0f;
	float translateX = 0.0f, translateY = 0.0f, translateZ = 0.0f;

	// 解析头部信息
	while (std::getline(file, line)) {
		// 跳过头部的注释
		if (line.substr(0, 7) == "comment") {
			// 提取缩放和平移信息
			if (line.find("frame_to_world_scale") != std::string::npos) {
				scale = std::stof(line.substr(line.find_last_of(" ") + 1));
			}
			if (line.find("frame_to_world_translation") != std::string::npos) {
				std::istringstream stream(line.substr(line.find_last_of(" ") + 1));
				stream >> translateX >> translateY >> translateZ;
			}
			continue;
		}

		// 解析顶点数量
		if (line.substr(0, 15) == "element vertex ") {
			vertexCount = std::stoi(line.substr(15));
		}

		// 找到头部结束
		if (line == "end_header") {
			inHeader = false;
			break;
		}
	}

	if (vertexCount == 0) {
		std::cerr << "No vertices found in PLY header!" << std::endl;
		return points;
	}

	// 读取点云数据
	while (std::getline(file, line)) {
		std::istringstream stream(line);
		Point p;

		// 读取每个点的坐标和颜色数据
		stream >> p.x >> p.y >> p.z >> p.r >> p.g >> p.b;

		// 应用缩放和平移操作
		p.x = (p.x + translateX) * scale;
		p.y = (p.y + translateY) * scale;
		p.z = (p.z + translateZ) * scale;
		p.r = p.r / 255.0f;
		p.g = p.g / 255.0f;
		p.b = p.b / 255.0f;
		points.push_back(p);

		// 如果已经读取到指定数量的顶点，停止读取
		if (points.size() >= vertexCount) {
			break;
		}
	}

	file.close();
	return points;
}
