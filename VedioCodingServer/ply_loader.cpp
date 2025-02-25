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

	// ����ͷ����Ϣ
	while (std::getline(file, line)) {
		// ����ͷ����ע��
		if (line.substr(0, 7) == "comment") {
			// ��ȡ���ź�ƽ����Ϣ
			if (line.find("frame_to_world_scale") != std::string::npos) {
				scale = std::stof(line.substr(line.find_last_of(" ") + 1));
			}
			if (line.find("frame_to_world_translation") != std::string::npos) {
				std::istringstream stream(line.substr(line.find_last_of(" ") + 1));
				stream >> translateX >> translateY >> translateZ;
			}
			continue;
		}

		// ������������
		if (line.substr(0, 15) == "element vertex ") {
			vertexCount = std::stoi(line.substr(15));
		}

		// �ҵ�ͷ������
		if (line == "end_header") {
			inHeader = false;
			break;
		}
	}

	if (vertexCount == 0) {
		std::cerr << "No vertices found in PLY header!" << std::endl;
		return points;
	}

	// ��ȡ��������
	while (std::getline(file, line)) {
		std::istringstream stream(line);
		Point p;

		// ��ȡÿ������������ɫ����
		stream >> p.x >> p.y >> p.z >> p.r >> p.g >> p.b;

		// Ӧ�����ź�ƽ�Ʋ���
		p.x = (p.x + translateX) * scale;
		p.y = (p.y + translateY) * scale;
		p.z = (p.z + translateZ) * scale;
		p.r = p.r / 255.0f;
		p.g = p.g / 255.0f;
		p.b = p.b / 255.0f;
		points.push_back(p);

		// ����Ѿ���ȡ��ָ�������Ķ��㣬ֹͣ��ȡ
		if (points.size() >= vertexCount) {
			break;
		}
	}

	file.close();
	return points;
}
