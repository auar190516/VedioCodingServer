#include <iostream>
#include <vector>
#include <filesystem>
#include "ply_loader.h"
#include "opengl_renderer.h"
#include "rtp_streamer.h"
#include <chrono>
#include <thread>
namespace fs = std::filesystem;
// 主函数
int main() {
	// 初始化 OpenGL 环境
	initializeOpenGL();
	// 文件夹路径
	std::string folderPath = "D:\\Graduate\\longdress\\longdress\\ply_downsample_001\\";
	std::string rtpURL = "rtp://127.0.0.1:5004";
	//std::string rtpURL = "b.mp4";
	RTPStreamer streamer(rtpURL, 480, 640);
	streamer.startStreaming();

	// 目标 FPS
	const int TARGET_FPS = 30;
	const int FRAME_TIME_MS = 1000 / TARGET_FPS;  // 计算目标帧时间 (毫秒)

	// 帧编号计数器
	//int frameNumber = 1;
		 //遍历文件夹中的所有文件
	for (const auto& entry : fs::directory_iterator(folderPath)) {
		auto frame_start = std::chrono::high_resolution_clock::now();  // 记录帧开始时间
		// 检查文件扩展名是否为 .ply
		if (entry.path().extension() == ".ply") {
			// 加载PLY文件
			std::string filename = entry.path().string();
			std::vector<Point> points = loadPLY(filename);

			// 如果加载失败或者没有点数据，跳过该文件
			if (points.empty()) {
				std::cerr << "Failed to load PLY file or no points found in " << filename << std::endl;
				continue;
			}

			//std::cout << "Loaded " << points.size() << " points from " << filename << std::endl;

			// 渲染点云并保存，使用当前帧编号
			std::vector<unsigned char> frameBuffer;
			renderPoints(points, frameBuffer);
			std::cout << frameBuffer.size() << std::endl;
			streamer.sendFrame(frameBuffer);
			// 增加帧编号
			//frameNumber++;
			// 控制帧率
			
		}
		auto frame_end = std::chrono::high_resolution_clock::now();
		int elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(frame_end - frame_start).count();
		int sleep_time = FRAME_TIME_MS - elapsed_time;
		if (sleep_time > 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));  // 让线程休眠
		}
	}


	// 退出 OpenGL
	cleanupOpenGL();
	streamer.stopStreaming();
	return 0;
}
