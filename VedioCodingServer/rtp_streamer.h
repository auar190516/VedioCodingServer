#pragma once
#ifndef RTP_STREAMER_H
#define RTP_STREAMER_H

#include <iostream>
#include <vector>
#include <string>
#include <stdio.h>

class RTPStreamer {
public:
	RTPStreamer(const std::string& rtp_url, int width, int height);
	~RTPStreamer();

	void startStreaming();
	void sendFrame(const std::vector<unsigned char>& frameData);
	void stopStreaming();

private:
	std::string rtpURL;
	int frameWidth;
	int frameHeight;
	FILE* ffmpegPipe;
};

#endif // RTP_STREAMER_H
