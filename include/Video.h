#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define VIDEO_PACKET_DURATION 1000
#define VIDEO_DEFAULT_FPS 60
#define VIDEO_DEFAULT_SAMPLE_COUNT 1152

typedef struct Video {
	char* filename;
	AVFormatContext* inputContext;
	AVFormatContext* outputContext;
	AVCodec* videoCodec;
	AVCodec* audioCodec;
	AVStream* inputStream;
	AVStream* outputStream;
	AVCodecContext* videoCodecContext_I; // Input
	AVCodecContext* audioCodecContext_I; // Input
	AVCodecContext* videoCodecContext_O; // Output
	AVCodecContext* audioCodecContext_O; // Output
	int videoStream;
	int audioStream;
	SwrContext* swrContext;
	struct SwsContext* swsContext;
	uint8_t* rgbBuffer;
	int lastRGBAvg;
	float lastAudioAvg;
	int** frames;
	int numClips;
	int clipIndex;
} Video;

AVStream* getVideoStream(Video* video);

AVStream* getAudioStream(Video* video);

void initVideo(Video* video, char* filename);

void initFrameArray(Video* video, int numClips);

int initResampler(AVCodecContext* inputContext, AVCodecContext* outputContext, SwrContext** resampleContext);

int openVideo(Video* video, char* outputFile);

int writeHeader(AVFormatContext* outputFormatContext);

int prepareStreamInfo(AVCodecContext** codecContext, AVCodec** codec, AVStream* stream);

int prepareVideoOutStream(Video* video);

int prepareAudioOutStream(Video* video);

int prepareVideoOutput(Video* video);

int findStreams(Video* video, char* outputFile);

int decodeVideo(Video* video, AVPacket* packet);

int encodeVideo(Video* video, AVFrame* frame);

int decodeAudio(Video* video, AVPacket* packet);

int encodeAudio(Video* video, AVFrame* frame);

int copyVideoFrames(Video* video);

int transcodeVideo(Video* video);

int findPacket(AVFormatContext* inputContext, int frameIndex, int stream);

bool isFrameInteresting(Video* video, AVFrame* frame, int stream, double threshold);

void initSwsContext(int height, int width, uint8_t** buffer, struct SwsContext** swsContext);

AVFrame* getRGBFrame(Video* video, AVFrame* frame);

void populateFrameArray(Video* video, int frameIndex);

void freeVideo(Video* video);
