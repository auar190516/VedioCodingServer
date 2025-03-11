#include "rtp_streamer.h"

RTPStreamer::RTPStreamer(const std::string& rtp_url, int width, int height)
	: rtpURL(rtp_url), frameWidth(width), frameHeight(height), ffmpegPipe(nullptr) {}

void RTPStreamer::startStreaming() {
	std::string command = "ffmpeg -f rawvideo -pixel_format rgb24 -video_size " +
		std::to_string(frameWidth) + "x" + std::to_string(frameHeight) +
		" -i - -vf vflip -c:v h264_nvenc  -preset fast "
		"-pix_fmt yuv420p -f rtp " + rtpURL;
	/*std::string command = "ffmpeg -f rawvideo -pixel_format rgb24 -video_size " +
		std::to_string(frameWidth) + "x" + std::to_string(frameHeight) +
		" -i - -vf vflip -c:v h264_nvenc -preset fast -pix_fmt yuv420p "
		"-movflags faststart -y " + rtpURL;*/
	ffmpegPipe = _popen(command.c_str(), "wb");
	if (!ffmpegPipe) {
		std::cerr << "Failed to start FFmpeg RTP streaming!" << std::endl;
		exit(1);
	}
	//ffmpegPipe = _popen(command.c_str(), "wb"); // "wb" 以二进制写入
	//if (!ffmpegPipe) {
	//	std::cerr << "Failed to start FFmpeg for MP4 recording!" << std::endl;
	//	exit(1);
	//}
}

void RTPStreamer::sendFrame(const std::vector<unsigned char>& frameData) {
	if (ffmpegPipe) {
		fwrite(frameData.data(), 1, frameWidth * frameHeight * 3, ffmpegPipe);
		fflush(ffmpegPipe);
	}

}

void RTPStreamer::stopStreaming() {
	if (ffmpegPipe) {
		_pclose(ffmpegPipe);
		ffmpegPipe = nullptr;
	}
	//if (ffmpegPipe) {
	//	fwrite("\x00\x00\x00\x01", 1, 4, ffmpegPipe); // 发送空数据确保 FFmpeg 正确结束
	//	_pclose(ffmpegPipe);
	//	ffmpegPipe = nullptr;
	//}
}

RTPStreamer::~RTPStreamer() {
	stopStreaming();
}
