#include <stdlib.h>
#include "ClipSequence.h"

int main(int argc, char* argv[]) {
	ClipSequence* sequence = (ClipSequence*)malloc(sizeof(ClipSequence));
	initSequence(&sequence);

	Video* video = (Video*)malloc(sizeof(Video));
	initVideo(video, argv[1]);

	if (allocateSequenceOutput(sequence, argv[2]) < 0) {
		printf("[ERROR] Failed to allocate output for %s\n", argv[4]);
		return -1;
	}

	if (openCurrentVideo(sequence, video) < 0) {
		printf("[ERROR] Video %s failed to open\n", video->filename);
		return -1;
	}
	if (findVideoStreams(sequence, video) < 0) {
		printf("[ERROR] Could not find streams\n");
		return -1;
	}

	writeHeader(sequence->outputContext);
	if (prepareVideoOutput(video) < 0) {
		printf("[ERROR] Failed to prepare video output\n");
		return -1;
	}
	int startFrame = atoi(argv[3]);
	int endFrame = atoi(argv[4]);
	if (cutVideo(sequence, video, startFrame, endFrame) < 0) {
		printf("[ERROR] Failed to cut video from frame %i to frame %i\n", startFrame, endFrame);
		return -1;
	}
	av_write_trailer(sequence->outputContext);
	freeVideo(video);
	freeSequence(sequence);
	return 0;
}
