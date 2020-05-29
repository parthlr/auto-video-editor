#include <dirent.h>
#include <stdio.h>
#include "VideoList.h"

typedef struct ClipSequence {
	VideoList* videos;
	AVFormatContext* outputContext;
	AVStream* outputStream;
	int64_t v_lastdts, a_lastdts;
	int64_t v_currentdts, a_currentdts;
} ClipSequence;

//void initVideoList(VideoList* videos);

void initSequence(ClipSequence** sequence);

int addVideo(ClipSequence* sequence, Video* video);

int openCurrentVideo(ClipSequence* sequence, Video* video);

int allocateSequenceOutput(ClipSequence* sequence, char* outputFile);

int findVideoStreams(ClipSequence* sequence, Video* video);

int decodeVideoSequence(ClipSequence* sequence, Video* video, AVPacket* packet, AVFrame* frame);

int encodeVideoSequence(ClipSequence* sequence, Video* video, AVFrame* frame);

int decodeAudioSequence(ClipSequence* sequence, Video* video, AVPacket* packet, AVFrame* frame);

int encodeAudioSequence(ClipSequence* sequence, Video* video, AVFrame* frame);

int readSequenceFrames(ClipSequence* sequence, Video* video, AVPacket* packet, AVFrame* frame);

void freeSequence(ClipSequence* sequence);
