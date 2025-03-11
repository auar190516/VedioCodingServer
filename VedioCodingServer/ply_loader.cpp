#include "ply_loader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

std::vector<Point> loadPLY(const std::string& filename) {
	std::ifstream file(filename, std::ios::binary); // 以二进制模式打开
	if (!file.is_open()) {
		std::cerr << "Failed to open file: " << filename << std::endl;
		return {};
	}

	std::string line;
	std::vector<Point> points;
	bool isBinary = false;
	int vertexCount = 0;
	float scale = 0.179523f;
	float translateX = -45.2095f, translateY = 7.18301f, translateZ = -54.3561f;

	// 解析 PLY 头部
	while (std::getline(file, line)) {
		if (line.find("format binary_little_endian") != std::string::npos) {
			isBinary = true;
		}
		else if (line.find("format ascii") != std::string::npos) {
			std::cerr << "ERROR: ASCII PLY files are not supported!" << std::endl;
			return {};
		}

		if (line.substr(0, 15) == "element vertex ") {
			vertexCount = std::stoi(line.substr(15));
		}

		if (line == "end_header") {
			break; // 头部解析完毕
		}
	}

	if (!isBinary) {
		std::cerr << "ERROR: Only binary PLY format is supported!" << std::endl;
		return {};
	}

	if (vertexCount == 0) {
		std::cerr << "ERROR: No vertices found in PLY header!" << std::endl;
		return {};
	}

	// 现在文件指针在 "end_header" 之后，直接读取二进制数据
	//points.reserve(vertexCount);
	points.reserve(vertexCount);
	for (int i = 0; i < vertexCount; ++i) {
		Point p;
		double dx, dy, dz; // 先用 double 读取 8 字节数据

		// 读取 double 类型坐标（8 字节）
		file.read(reinterpret_cast<char*>(&dx), sizeof(double));
		file.read(reinterpret_cast<char*>(&dy), sizeof(double));
		file.read(reinterpret_cast<char*>(&dz), sizeof(double));

		// 转换为 float 类型
		p.x = static_cast<float>(dx);
		p.y = static_cast<float>(dy);
		p.z = static_cast<float>(dz);
		p.x = p.x * scale + translateX;
		p.y = p.y * scale + translateY;
		p.z = p.z * scale + translateZ;
		// 读取 uint8_t 类型颜色
		uint8_t r, g, b;
		file.read(reinterpret_cast<char*>(&r), sizeof(uint8_t));
		file.read(reinterpret_cast<char*>(&g), sizeof(uint8_t));
		file.read(reinterpret_cast<char*>(&b), sizeof(uint8_t));

		// 归一化颜色值
		p.r = r / 255.0f;
		p.g = g / 255.0f;
		p.b = b / 255.0f;

		points.push_back(p);
	}
	file.close();
	return points;
}
