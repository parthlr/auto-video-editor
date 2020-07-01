#include "Video.h"

int main(int argc, char* argv[]) {
	Video* video = (Video*)malloc(sizeof(Video));
	initVideo(video, argv[1]);
	if (findStreams(video, argv[2]) < 0) {
		printf("[ERROR] Could not find streams\n");
		return -1;
	}

	writeHeader(video->outputContext);

	// Uncomment if transcoding
	/*if (prepareVideoOutput(video) < 0) {
		printf("[ERROR] Failed to prepare video output\n");
		return -1;
	}
	if (transcodeVideo(video) < 0) {
		printf("[ERROR] Failed to transcode video\n");
		return -1;
	}*/
	
	if (copyVideoFrames(video) < 0) {
		printf("[ERROR] Failed to read and write new video\n");
		return -1;
	}
	av_write_trailer(video->outputContext);
	freeVideo(video); // Frees all codecs and contexts
	return 0;
}
