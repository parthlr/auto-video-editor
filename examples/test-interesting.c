#include <stdlib.h>
#include "ClipSequence.h"

int main(int argc, char* argv[]) {
	ClipSequence* sequence = (ClipSequence*)malloc(sizeof(ClipSequence));
	initSequence(&sequence);

	int numClips = atoi(argv[3]);

	Video* video = (Video*)malloc(sizeof(Video));
	initVideo(video, argv[1]);
	initFrameArray(video, numClips);

	if (allocateSequenceOutput(sequence, argv[2]) < 0) {
		printf("[ERROR] Failed to allocate output for %s\n", argv[2]);
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

	if (copySequenceFrames(sequence, video, false) < 0) {
		printf("[ERROR] Failed to read and write new video\n");
		return -1;
	}

	printf("Done analyzing video\n");

	writeHeader(sequence->outputContext);
	if (prepareVideoOutput(video) < 0) {
		printf("[ERROR] Failed to prepare video output\n");
		return -1;
	}

	for (int i = 0; i < video->clipIndex; i++) {
		cutVideo(sequence, video, (video->frames)[i][0], (video->frames)[i][1]);
	}

	av_write_trailer(sequence->outputContext);

	freeVideo(video);
	freeSequence(sequence);
}
