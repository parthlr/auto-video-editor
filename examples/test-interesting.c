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

	// Comment out if using dummy values
	/*if (analyzeVideo(sequence, video) < 0) {
		printf("[ERROR] Failed to read and write new video\n");
		return -1;
	}*/

	// Dummy values for testing without having having to analyze video
	(video->frames)[0][0] = 0;
	(video->frames)[0][1] = 370;
	(video->frames)[1][0] = 1205;
	(video->frames)[1][1] = 1520;
	(video->frames)[2][0] = 10202;
	(video->frames)[2][1] = 12050;
	(video->frames)[3][0] = 36283;
	(video->frames)[3][1] = 36590;
	(video->frames)[4][0] = 40765;
	(video->frames)[4][1] = 41234;
	(video->frames)[5][0] = 50123;
	(video->frames)[5][1] = 51654;
	video->clipIndex = 6;

	printf("Done analyzing video\n");

	writeHeader(sequence->outputContext);
	/*if (prepareVideoOutput(video) < 0) {
		printf("[ERROR] Failed to prepare video output\n");
		return -1;
	}*/

	for (int i = 0; i < video->clipIndex; i++) {
		cutVideo(sequence, video, (video->frames)[i][0], (video->frames)[i][1]);
	}

	av_write_trailer(sequence->outputContext);

	freeVideo(video);
	freeSequence(sequence);
}
