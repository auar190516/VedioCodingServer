#include <iostream>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <filesystem>
#include <unordered_map>
#include <atomic>
#include <sstream>
#include <fstream>

#include "ply_loader.h"
#include "opengl_renderer.h"
#include "rtp_streamer.h"

#include <chrono>
#include <thread>

#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma  comment(lib, "Ws2_32.lib")
namespace fs = std::filesystem;
// 结构体管理客户端信息
struct ClientInfo {
	SOCKET socket;
	int rtpPort;
	bool is3D;  // 是否请求 3D 数据
	bool active;
	double posX, posY, posZ;//客户端位置
};
std::vector<Point> currentFrameData;
std::mutex dataMutex;
std::condition_variable dataCV;
bool newDataAvailable = false;
bool stopProcessing = false;

std::mutex renderNumMutex;
int renderCount = 0;
std::condition_variable ReadFileCV;
//int num_render_thread = 0;//一共的渲染线程

std::mutex clientMutex;
std::unordered_map<SOCKET, struct ClientInfo> clientStatus;
const int SERVER_PORT = 8888;
std::atomic<bool> serverRunning(true);

std::atomic<int> currentFrameIndex = 0;

const std::string drcFolderPath = "D:\\Graduate\\longdress\\longdress\\drc_001\\";
void fileReaderThread(const std::string& folderPath) {

	const int TARGET_FRAME_TIME_MS = 100;

	for (const auto& entry : fs::directory_iterator(folderPath)) {
		if (entry.path().extension() == ".ply") {
			auto frame_start = std::chrono::high_resolution_clock::now();
			std::vector<Point> points = loadPLY(entry.path().string());

			if (points.empty()) {
				std::cerr << "Failed to load PLY: " << entry.path().string() << std::endl;
				continue;
			}

			{
				std::unique_lock<std::mutex> lock(renderNumMutex);
				ReadFileCV.wait(lock, [] { return renderCount == 0; });  // 确保上一帧已被所有线程处理完
				currentFrameData = points;
				newDataAvailable = true;
				{
					std::unique_lock<std::mutex> lock(clientMutex);
					renderCount = static_cast<int>(clientStatus.size());  // 重置渲染计数
				}
			}
			currentFrameIndex++;
			dataCV.notify_all();  // 广播通知所有渲染线程

			auto frame_end = std::chrono::high_resolution_clock::now();
			int elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(frame_end - frame_start).count();
			int sleep_time = TARGET_FRAME_TIME_MS - elapsed_time;

			if (sleep_time > 0) {
				std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
			}
		}
	}

	{
		std::lock_guard<std::mutex> lock(dataMutex);
		stopProcessing = true;
	}
	dataCV.notify_all();  // 让所有线程退出
}

void renderThread(SOCKET clientSocket) {
	OpenGLContext context = initializeOpenGL();  // 每个线程独立创建 OpenGL 上下文
	//int rtpPort;
	//bool is3D;
	/*{
		std::lock_guard<std::mutex> lock(clientMutex);
		rtpPort = clientStatus[clientSocket].rtpPort;
		is3D = clientStatus[clientSocket].is3D;
	}*/
	std::string rtpURL = "rtp://127.0.0.1:" + std::to_string(5004); //暂时使用5004
	RTPStreamer streamer(rtpURL, 480, 640);
	streamer.startStreaming();
	while (serverRunning) {
		{
			std::lock_guard<std::mutex> lock(clientMutex);
			if (clientStatus.find(clientSocket) == clientStatus.end() || //计数要--
				!clientStatus[clientSocket].active) {
					{
						std::lock_guard<std::mutex> lock(renderNumMutex);
						renderCount--;
						if (renderCount == 0) {  // 所有线程都获取了这帧数据
							newDataAvailable = false;
							ReadFileCV.notify_one();  // 通知 fileReaderThread 继续读取
						}
					}
				break;
			}
		}

		{
			std::unique_lock<std::mutex> lock(dataMutex);
			dataCV.wait(lock, [] { return newDataAvailable || stopProcessing; });
			if (stopProcessing) {
				break;
			}
		}
		
		bool trans3D = false;
		{
			std::lock_guard<std::mutex> lock(clientMutex);
			trans3D = clientStatus[clientSocket].is3D;
		}
		if (trans3D) {
			// 如果需要传输 3D 点云
			// 根据 currentFrameIndex 生成 DRC 文件路径
			std::string drcFileName = std::to_string(1051 + currentFrameIndex) + ".drc";
			std::string drcFilePath = drcFolderPath + drcFileName;
			// 打开文件
			std::ifstream file(drcFilePath, std::ios::binary);
			if (!file.is_open()) {
				std::cerr << "Failed to open DRC file: " << drcFilePath << std::endl;
				return;
			}
			// 获取文件大小
			std::streamsize fileSize = file.tellg();
			file.seekg(0, std::ios::beg); 
			int bytesSent = send(clientSocket, reinterpret_cast<const char*>(&fileSize), sizeof(fileSize), 0);
			if (bytesSent == SOCKET_ERROR) {
				std::cerr << "Failed to send file size" << std::endl;
				file.close();
				return;
			}
			// 读取文件并分块发送
			char buffer[1024];
			while (!file.eof()) {
				file.read(buffer, sizeof(buffer));
				size_t bytesRead = file.gcount();
				int bytesSent = send(clientSocket, buffer, bytesRead, 0);
				if (bytesSent == SOCKET_ERROR) {
					std::cerr << "Failed to send DRC file" << std::endl;
					break;
				}
			}

			// 关闭文件
			file.close();
			std::cout << "DRC file sent successfully." << std::endl;
		}
		else {
			std::vector<Point> localFrameData;
			localFrameData = currentFrameData;  // 复制数据
			std::vector<unsigned char> frameBuffer;
			renderPoints(context, localFrameData, frameBuffer);
			streamer.sendFrame(frameBuffer);

			{
				std::lock_guard<std::mutex> lock(renderNumMutex);
				renderCount--;
				if (renderCount == 0) {  // 所有线程都获取了这帧数据
					newDataAvailable = false;
					ReadFileCV.notify_one();  // 通知 fileReaderThread 继续读取
				}
			}
		}
	}

	streamer.stopStreaming();
	cleanupOpenGL(context);
	/*{
		std::lock_guard<std::mutex> lock(clientMutex);
		clientStatus.erase(clientSocket);
	}*/
}
// 客户端消息接收线程：接收客户端发送的控制命令（如位置信息、模式切换）并更新状态
void clientMessageThread(SOCKET clientSocket) {
	char buffer[1024];
	while (serverRunning) {
		int recvLen = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
		if (recvLen <= 0) {
			// 客户端关闭连接或发生错误，更新状态
			std::lock_guard<std::mutex> lock(clientMutex);
			if (clientStatus.find(clientSocket) != clientStatus.end())
				clientStatus[clientSocket].active = false;
			break;
		}
		buffer[recvLen] = '\0';
		std::istringstream iss(buffer);
		std::string command;
		iss >> command;
		if (command == "LOCATION") {
			double x, y, z;
			iss >> x >> y >> z;
			{
				std::lock_guard<std::mutex> lock(clientMutex);
				if (clientStatus.find(clientSocket) != clientStatus.end()) {
					clientStatus[clientSocket].posX = x;
					clientStatus[clientSocket].posY = y;
					clientStatus[clientSocket].posZ = z;
				}
			}
			std::cout << "Received location from client " << clientSocket
				<< ": (" << x << ", " << y << ", " << z << ")\n";
		}
		else if (command == "MODE") {
			std::string mode;
			iss >> mode;
			{
				std::lock_guard<std::mutex> lock(clientMutex);
				if (clientStatus.find(clientSocket) != clientStatus.end()) {
					clientStatus[clientSocket].is3D = (mode == "3D");
				}
			}
			std::cout << "Client " << clientSocket << " set mode to "
				<< mode << "\n";
		}
		// 可根据需要添加更多命令处理逻辑
	}
	// 关闭 socket 并从客户端集合中移除
	closesocket(clientSocket);
	std::lock_guard<std::mutex> lock(clientMutex);
	clientStatus.erase(clientSocket);
}
// 服务器线程
void serverThread() {
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == INVALID_SOCKET) {
		std::cerr << "Failed to create server socket\n";
		return;
	}

	sockaddr_in serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(SERVER_PORT);

	if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "Bind failed\n";
		return;
	}

	if (listen(serverSocket, 5) == SOCKET_ERROR) {
		std::cerr << "Listen failed\n";
		return;
	}

	std::cout << "Server listening on port " << SERVER_PORT << std::endl;

	while (serverRunning) {
		sockaddr_in clientAddr;
		int clientLen = sizeof(clientAddr);
		SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientLen);
		if (clientSocket == INVALID_SOCKET) {
			std::cerr << "Failed to accept connection\n";
			continue;
		}

		int rtpPort = 5000 + clientSocket % 1000;
		{
			std::lock_guard<std::mutex> lock(clientMutex);
			clientStatus[clientSocket] = { clientSocket, rtpPort, false, true, 0, 0, 0 };
		}

		std::cout << "New client connected: " << clientSocket << ", RTP Port: " << rtpPort << std::endl;
		std::thread(renderThread, clientSocket).detach();
	}

	closesocket(serverSocket);
	WSACleanup();
}

// 主函数
int main() {
	// 文件夹路径
	std::string folderPath = "D:\\Graduate\\longdress\\longdress\\ply_downsample_001\\";
	//std::string rtpURL = "rtp://127.0.0.1:5004";
	//std::string rtpURL = "b.mp4";
	//RTPStreamer streamer(rtpURL, 480, 640);
	//streamer.startStreaming();

	//// 目标 FPS
	//const int TARGET_FPS = 30;
	//const int FRAME_TIME_MS = 1000 / TARGET_FPS;  // 计算目标帧时间 (毫秒)
	std::thread readerThread(fileReaderThread, folderPath);
	std::thread server(serverThread);

	readerThread.join();
	serverRunning = false;

	{
		std::lock_guard<std::mutex> lock(clientMutex);
		for (auto& [client, thread] : clientStatus) {
			closesocket(client);
		}
	}

	server.join();

	return 0;
}
