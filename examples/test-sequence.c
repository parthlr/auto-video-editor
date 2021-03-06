#include "ClipSequence.h"

int main(int argc, char* argv[]) {
	ClipSequence* sequence = (ClipSequence*)malloc(sizeof(ClipSequence));

	initSequence(&sequence);

	Video* video = (Video*)malloc(sizeof(Video));
	Video* video2 = (Video*)malloc(sizeof(Video));
	Video* video3 = (Video*)malloc(sizeof(Video));

	initVideo(video, argv[1]);
	initVideo(video2, argv[1]);
	initVideo(video3, argv[1]);

	addVideo(sequence, video);
	addVideo(sequence, video2);
	addVideo(sequence, video3);

	if (allocateSequenceOutput(sequence, argv[2]) < 0) {
		printf("[ERROR] Failed to allocate output for %s\n", argv[2]);
		return -1;
	}

	Node* currentNode = (Node*)malloc(sizeof(Node));
	currentNode = sequence->videos->head;
	int index = 0;
	while (currentNode != NULL) {
		if (openCurrentVideo(sequence, currentNode->video) < 0) {
			printf("[ERROR] Video %s failed to open\n", currentNode->video->filename);
			return -1;
		}
		if (findVideoStreams(sequence, currentNode->video) < 0) {
			printf("[ERROR] Could not find streams\n");
			return -1;
		}
		if (index == 0) {
			writeHeader(sequence->outputContext);
		}

		// Uncomment if transcoding
		/*if (prepareVideoOutput(video) < 0) {
			printf("[ERROR] Failed to prepare video output\n");
			return -1;
		}
		if (transcodeSequence(sequence, currentNode->video) < 0) {
			printf("[ERROR] Failed to transcode new video\n");
			return -1;
		}*/

		if (copySequenceFrames(sequence, currentNode->video) < 0) {
			printf("[ERROR] Failed to copy new video\n");
			return -1;
		}
		freeVideo(currentNode->video); // Frees all codecs and contexts
		currentNode = currentNode->next;
		index++;
	}

	av_write_trailer(sequence->outputContext);

	freeSequence(sequence);
	return 0;
}
