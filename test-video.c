#include "Video.h"

int main(int argc, char* argv[]) {
	Video* video = (Video*)malloc(sizeof(Video));
	initVideo(video);
	if (findStreams(video, argv[1], argv[2]) < 0) {
		printf("[ERROR] Could not find streams\n");
		return -1;
	}

	AVDictionary* dic = NULL;
	if (avformat_write_header(video->outputContext, &dic) < 0) {
		printf("[ERROR] Error while writing header to output file\n");
		return -1;
	}
	AVFrame* frame = av_frame_alloc();
	AVPacket* packet = av_packet_alloc();
	if (readFrames(video, packet, frame) < 0) {
		printf("[ERROR] Failed to read and write new video\n");
		return -1;
	}
	av_write_trailer(video->outputContext);
	freeVideo(video); // Frees all codecs and contexts
	return 0;
}
