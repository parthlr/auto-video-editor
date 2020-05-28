#include "ClipSequence.h"

int main(int argc, char* argv[]) {
	Video* video = (Video*)malloc(sizeof(Video));
	initVideo(video, argv[1]);
	ClipSequence* sequence = (ClipSequence*)malloc(sizeof(ClipSequence));
	initSequence(&sequence);
	//addVideo(sequence, video);
	if (openCurrentVideo(sequence, video) < 0) {
		printf("[ERROR] Video %s failed to open\n", video->filename);
		return -1;
	}
	if (allocateSequenceOutput(sequence, argv[2]) < 0) {
		printf("[ERROR] Failed to allocate output for %s\n", argv[2]);
		return -1;
	}
	if (findVideoStreams(sequence, video) < 0) {
		printf("[ERROR] Could not find streams\n");
		return -1;
	}
	if (!(sequence->outputContext->oformat->flags & AVFMT_NOFILE)) {
    if (avio_open(&(sequence->outputContext->pb), argv[2], AVIO_FLAG_WRITE) < 0) {
      printf("Could not open output file %s", argv[2]);
      return -1;
    }
  }
	AVDictionary* dic = NULL;
	if (avformat_write_header(sequence->outputContext, &dic) < 0) {
		printf("[ERROR] Error while writing header to output file\n");
		return -1;
	}
	AVFrame* frame = av_frame_alloc();
	AVPacket* packet = av_packet_alloc();
	if (readSequenceFrames(sequence, video, packet, frame) < 0) {
		printf("[ERROR] Failed to read and write new video\n");
		return -1;
	}
	freeVideo(video); // Frees all codecs and contexts

	Video* video2 = (Video*)malloc(sizeof(Video));
	initVideo(video2, argv[1]);
	if (openCurrentVideo(sequence, video2) < 0) {
		printf("[ERROR] Video %s failed to open\n", video2->filename);
		return -1;
	}
	if (findVideoStreams(sequence, video2) < 0) {
		printf("[ERROR] Could not find streams\n");
		return -1;
	}
	/*if (!(sequence->outputContext->oformat->flags & AVFMT_NOFILE)) {
    if (avio_open(&(sequence->outputContext->pb), argv[2], AVIO_FLAG_WRITE) < 0) {
      printf("Could not open output file %s", argv[2]);
      return -1;
    }
  }*/
	AVFrame* frame2 = av_frame_alloc();
	AVPacket* packet2 = av_packet_alloc();
	if (readSequenceFrames(sequence, video2, packet2, frame2) < 0) {
		printf("[ERROR] Failed to read and write new video\n");
		return -1;
	}
	freeVideo(video2); // Frees all codecs and contexts

	Video* video3 = (Video*)malloc(sizeof(Video));
	initVideo(video3, argv[1]);
	if (openCurrentVideo(sequence, video3) < 0) {
		printf("[ERROR] Video %s failed to open\n", video3->filename);
		return -1;
	}
	if (findVideoStreams(sequence, video3) < 0) {
		printf("[ERROR] Could not find streams\n");
		return -1;
	}
	/*if (!(sequence->outputContext->oformat->flags & AVFMT_NOFILE)) {
    if (avio_open(&(sequence->outputContext->pb), argv[2], AVIO_FLAG_WRITE) < 0) {
      printf("Could not open output file %s", argv[2]);
      return -1;
    }
  }*/
	AVFrame* frame3 = av_frame_alloc();
	AVPacket* packet3 = av_packet_alloc();
	if (readSequenceFrames(sequence, video3, packet3, frame3) < 0) {
		printf("[ERROR] Failed to read and write new video\n");
		return -1;
	}
	freeVideo(video3); // Frees all codecs and contexts

	av_write_trailer(sequence->outputContext);


	return 0;
}
