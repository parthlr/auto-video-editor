#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <string.h>

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
} Video;

AVStream* getVideoStream(Video* video);

AVStream* getAudioStream(Video* video);

void initVideo(Video* video, char* filename);

int initResampler(AVCodecContext* inputContext, AVCodecContext* outputContext, SwrContext** resampleContext);

int openVideo(Video* video, char* outputFile);

int writeHeader(AVFormatContext* outputFormatContext);

int prepareStreamInfo(AVCodecContext** codecContext, AVCodec** codec, AVStream* stream);

int prepareVideoOutStream(Video* video);

int prepareAudioOutStream(Video* video);

int findStreams(Video* video, char* outputFile);

int decodeVideo(Video* video, AVPacket* packet, AVFrame* frame);

int encodeVideo(Video* video, AVFrame* frame);

int decodeAudio(Video* video, AVPacket* packet, AVFrame* frame);

int encodeAudio(Video* video, AVFrame* frame);

int readFrames(Video* video, AVPacket* packet, AVFrame* frame);

void freeVideo(Video* video);
