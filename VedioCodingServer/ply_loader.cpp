#include "ply_loader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

std::vector<Point> loadPLY(const std::string& filename) {
	std::ifstream file(filename); // �Զ�����ģʽ��
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
		// ���� PLY ͷ��
		while (std::getline(file, line)) {
			if (line.find("format binary_little_endian") != std::string::npos) {
				isBinary = true;
			}
			else if (line.find("format ascii") != std::string::npos) {
				isBinary = false; // ���Ϊ ASCII
			}

			if (line.substr(0, 15) == "element vertex ") {
				vertexCount = std::stoi(line.substr(15));
			}

			if (line == "end_header") {
				break; // ͷ���������
			}
		}

		if (vertexCount == 0) {
			std::cerr << "ERROR: No vertices found in PLY header!" << std::endl;
			return {};
		}
		points.reserve(vertexCount);

		if (isBinary) {
			std::cerr << "ERROR: This function only supports ASCII PLY files!" << std::endl;
			return {};
		}

		// ���� ASCII ��ʽ�� PLY �ļ�
		for (int i = 0; i < vertexCount; ++i) {
			Point p;
			float r, g, b;
			if (!(file >> p.x >> p.y >> p.z >> r >> g >> b)) {
				std::cerr << "ERROR: Malformed PLY file!" << std::endl;
				return {};
			}
			p.x = p.x * scale + translateX;
			p.y = p.y * scale + translateY;
			p.z = p.z * scale + translateZ;
			// ��һ����ɫ
			p.r = r / 255.0f;
			p.g = g / 255.0f;
			p.b = b / 255.0f;

			points.push_back(p);
		}

	file.close();
	return points;
}
