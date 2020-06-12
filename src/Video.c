#include "Video.h"

void initVideo(Video* video, char* filename) {
	video->inputContext = NULL;
	video->outputContext = NULL;
	video->videoCodec = NULL;
	video->audioCodec = NULL;
	video->inputStream = NULL;
	video->outputStream = NULL;
	video->videoCodecContext_I = NULL;
	video->audioCodecContext_I = NULL;
	video->videoStream = -1;
	video->audioStream = -1;
	video->filename = (char*)malloc(strlen(filename) + 1);
	strcpy(video->filename, filename);
	video->lastRGBAvg = 127; // Middle RGB value
	video->lastAudioAvg = 0.5; // Middle audio sample
	video->clipIndex = 0;
}

void initFrameArray(Video* video, int numClips) {
	video->numClips = numClips;
	video->frames = (int**)malloc(numClips * sizeof(int*));
	for (int i = 0; i < numClips; i++) {
		(video->frames)[i] = (int*)malloc(2 * sizeof(int));
	}
}

int initResampler(AVCodecContext* inputContext, AVCodecContext* outputContext, SwrContext** resampleContext) {
	*resampleContext = swr_alloc_set_opts(NULL, av_get_default_channel_layout(outputContext->channels), outputContext->sample_fmt, outputContext->sample_rate, av_get_default_channel_layout(inputContext->channels), inputContext->sample_fmt, inputContext->sample_rate, 0, NULL);
	if (!*resampleContext) {
		printf("[ERROR] Failed to allocate resample context\n");
		return -1;
	}
	if (swr_init(*resampleContext) < 0) {
		printf("[ERROR] Could not open resample context\n");
		return -1;
	}
	return 0;
}

AVStream* getVideoStream(Video* video) {
	int videoStream = video->videoStream;
	if (videoStream == -1) {
		return NULL;
	}
	return video->inputContext->streams[videoStream];
}

AVStream* getAudioStream(Video* video) {
	int audioStream = video->audioStream;
	if (audioStream == -1) {
		return NULL;
	}
	return video->inputContext->streams[audioStream];
}

int findStreams(Video* video, char* outputFile) {
	if (openVideo(video, outputFile) < 0) {
		printf("[ERROR] Video %s failed to open\n", video->filename);
		return -1;
	}
	for (int i = 0; i < video->inputContext->nb_streams; i++) {
		video->inputStream = video->inputContext->streams[i];
		if (video->inputContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			video->videoStream = i;
			if (prepareStreamInfo(&(video->videoCodecContext_I), &(video->videoCodec), video->inputStream) < 0) {
				printf("[ERROR] Could not prepare video stream information\n");
				return -1;video->outputStream->time_base = video->audioCodecContext_O->time_base;
			}
		} else if (video->inputContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			video->audioStream = i;
			if (prepareStreamInfo(&(video->audioCodecContext_I), &(video->audioCodec), video->inputStream) < 0) {
				printf("[ERROR] Could not prepare audio stream information\n");
				return -1;
			}
		}
		video->outputStream = avformat_new_stream(video->outputContext, NULL);
		if (!video->outputStream) {
			printf("[ERROR] Failed allocating output stream\n");
			return -1;
		}
		if (avcodec_parameters_copy(video->outputStream->codecpar, video->inputStream->codecpar) < 0) {
			printf("[ERROR] Failed to copy codec parameters\n");
			return -1;
		}
	}
	if (video->videoStream == -1) {
		printf("[ERROR] Video stream for %s not found\n", video->filename);
		return -1;
	}
	if (video->audioStream == -1) {
		printf("[ERROR] Audio stream for %s not found\n", video->filename);
		return -1;
	}
	if (!(video->outputContext->oformat->flags & AVFMT_NOFILE)) {
    if (avio_open(&(video->outputContext->pb), outputFile, AVIO_FLAG_WRITE) < 0) {
      printf("Could not open output file %s", outputFile);
      return -1;
    }
  }
	return 0;
}

int openVideo(Video* video, char* outputFile) {
	video->inputContext = avformat_alloc_context();
	if (!video->inputContext) {
		printf("[ERROR] Failed to allocate input format context\n");
		return -1;
	}
	if (avformat_open_input(&(video->inputContext), video->filename, NULL, NULL) < 0) {
		printf("[ERROR] Could not open the input file\n");
		return -1;
	}

	if (avformat_find_stream_info(video->inputContext, NULL) < 0) {
		printf("[ERROR] Failed to retrieve input stream info\n");
		return -1;
	}
	avformat_alloc_output_context2(&(video->outputContext), NULL, NULL, outputFile);
	if (!video->outputContext) {
		printf("[ERROR] Failed to create output context\n");
		return -1;
	}
	printf("[OPEN] Video %s opened\n", video->filename);
	return 0;
}

int writeHeader(AVFormatContext* outputFormatContext) {
	AVDictionary* dic = NULL;
	if (avformat_write_header(outputFormatContext, &dic) < 0) {
		printf("[ERROR] Error while writing header to output file\n");
		return -1;
	}
	return 0;
}

int prepareStreamInfo(AVCodecContext** codecContext, AVCodec** codec, AVStream* stream) {
	*codec = avcodec_find_decoder(stream->codecpar->codec_id);
	if (!*codec) {
		printf("[ERROR] Failed to find input codec\n");
		return -1;
	}
	*codecContext = avcodec_alloc_context3(*codec);
	if (!codecContext) {
		printf("[ERROR] Failed to allocate memory for input codec context\n");
		return -1;
	}
	if (avcodec_parameters_to_context(*codecContext, stream->codecpar) < 0) {
		printf("[ERROR] Failed to fill input codec context\n");
		return -1;
	}
	if (avcodec_open2(*codecContext, *codec, NULL) < 0) {
		printf("[ERROR] Failed to open input codec\n");
		return -1;
	}
	return 0;
}

int prepareVideoOutStream(Video* video) {
	video->videoCodec = avcodec_find_encoder(getVideoStream(video)->codecpar->codec_id);
	if (!video->videoCodec) {
		printf("[ERROR] Failed to find video output codec\n");
		return -1;
	}
	video->videoCodecContext_O = avcodec_alloc_context3(video->videoCodec);
	if (!video->videoCodecContext_O) {
		printf("[ERROR] Failed to allocate memory for video output codec context\n");
		return -1;
	}
	video->videoCodecContext_O->bit_rate = video->videoCodecContext_I->bit_rate;
	video->videoCodecContext_O->width = video->videoCodecContext_I->width;
	video->videoCodecContext_O->height = video->videoCodecContext_I->height;
	video->videoCodecContext_O->time_base = (AVRational){1, 60};
	video->videoCodecContext_O->framerate = (AVRational){60, 1};
	video->videoCodecContext_O->pix_fmt = video->videoCodecContext_I->pix_fmt;
	video->videoCodecContext_O->extradata = video->videoCodecContext_I->extradata;
	video->videoCodecContext_O->extradata_size = video->videoCodecContext_I->extradata_size;
	if (avcodec_open2(video->videoCodecContext_O, video->videoCodec, NULL) < 0) {
		printf("[ERROR] Failed to open video output codec\n");
		return -1;
	}
	if (avcodec_parameters_from_context(getVideoStream(video)->codecpar, video->videoCodecContext_O) < 0) {
		printf("[ERROR] Failed to fill video stream\n");
		return -1;
	}
	return 0;
}

int prepareAudioOutStream(Video* video) {
	video->audioCodec = avcodec_find_encoder_by_name("mp2");
	if (!video->audioCodec) {
		printf("[ERROR] Failed to find audio output codec\n");
		return -1;
	}
	video->audioCodecContext_O = avcodec_alloc_context3(video->audioCodec);
	if (!video->audioCodecContext_O) {
		printf("[ERROR] Failed to allocate memory for audio output codec context\n");
		return -1;
	}
	video->audioCodecContext_O->channels = video->audioCodecContext_I->channels;
	video->audioCodecContext_O->channel_layout = av_get_default_channel_layout(video->audioCodecContext_O->channels);
	video->audioCodecContext_O->sample_rate = video->audioCodecContext_I->sample_rate;
	video->audioCodecContext_O->sample_fmt = video->audioCodec->sample_fmts[0];
	video->audioCodecContext_O->bit_rate = 384000;//video->audioCodecContext_I->bit_rate;
	video->audioCodecContext_O->time_base = video->audioCodecContext_I->time_base;
	video->audioCodecContext_O->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
	if (avcodec_open2(video->audioCodecContext_O, video->audioCodec, NULL) < 0) {
		printf("[ERROR] Failed to open audio output codec\n");
		return -1;
	}
	if (avcodec_parameters_from_context(getAudioStream(video)->codecpar, video->audioCodecContext_O) < 0) {
		printf("[ERROR] Failed to fill audio stream\n");
		return -1;
	}
	return 0;
}

int prepareVideoOutput(Video* video) {
	if (prepareVideoOutStream(video) < 0) {
		printf("[ERROR] Failed to prepare output video stream\n");
		return -1;
	}
	if (prepareAudioOutStream(video) < 0) {
		printf("[ERROR] Failed to prepare output audio stream\n");
		return -1;
	}
	if (initResampler(video->audioCodecContext_I, video->audioCodecContext_O, &(video->swrContext)) < 0) {
		printf("[ERROR] Failed to init audio resampler\n");
		return -1;
	}
	return 0;
}

int copyVideoFrames(Video* video, AVPacket* packet, AVFrame* frame) {
	if (!packet) {
		printf("[ERROR] Packet not allocated to be read\n");
		return -1;
	}
	if (!frame) {
		printf("[ERROR] Frame not allocated to be read\n");
		return -1;
	}
	int frameNum = 0;
	while (av_read_frame(video->inputContext, packet) >= 0) {
		printf("[READ] Reading frame %i\n", frameNum);
		if (packet->stream_index == video->videoStream) {
			if (decodeVideo(video, packet, frame) < 0) {
				printf("[ERROR] Failed to decode and encode video\n");
				return -1;
			}
		} else if (packet->stream_index == video->audioStream) {
			if (decodeAudio(video, packet, frame) < 0) {
				printf("[ERROR] Failed to decode and encode audio\n");
				return -1;
			}
		}
		av_packet_unref(packet);
		frameNum++;
	}
	// Flush encoder
	encodeVideo(video, NULL);
	encodeAudio(video, NULL);
	return 0;
}

int decodeVideo(Video* video, AVPacket* packet, AVFrame* frame) {
	int response = avcodec_send_packet(video->videoCodecContext_I, packet);
	if (response < 0) {
		printf("[ERROR] Failed to send video packet to decoder\n");
		return response;
	}
	while (response >= 0) {
		response = avcodec_receive_frame(video->videoCodecContext_I, frame);
		if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
			break;
		} else if (response < 0) {
			printf("[ERROR] Failed to receive video frame from decoder\n");
			return response;
		}
		if (response >= 0) {
			// Do stuff and encode
			if (encodeVideo(video, frame) < 0) {
				printf("[ERROR] Failed to encode new video\n");
				return -1;
			}
		}
		av_frame_unref(frame);
	}
	return 0;
}

int encodeVideo(Video* video, AVFrame* frame) {
	AVPacket* packet = av_packet_alloc();
	if (!packet) {
		printf("[ERROR] Could not allocate memory for video output packet\n");
		return -1;
	}
	int response = avcodec_send_frame(video->videoCodecContext_O, frame);
	if (response < 0) {
		printf("[ERROR] Failed to send video frame for encoding\n");
		return response;
	}
	while (response >= 0) {
		response = avcodec_receive_packet(video->videoCodecContext_O, packet);
		if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
			break;
		} else if (response < 0) {
			printf("[ERROR] Failed to receive video packet from encoder\n");
			return response;
		}
		packet->stream_index = video->videoStream;
		video->inputStream = getVideoStream(video);
		video->outputStream = video->outputContext->streams[packet->stream_index];
		packet->pts = av_rescale_q_rnd(packet->pts, video->inputStream->time_base, video->outputStream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
		packet->dts = av_rescale_q_rnd(packet->dts, video->inputStream->time_base, video->outputStream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
		packet->duration = av_rescale_q(packet->duration, video->inputStream->time_base, video->outputStream->time_base);
		packet->pos = -1;

		response = av_interleaved_write_frame(video->outputContext, packet);
		if (response < 0) {
			printf("[ERROR] Failed to write video packet\n");
			break;
		}
	}
	av_packet_unref(packet);
	av_packet_free(&packet);
	return 0;
}

int decodeAudio(Video* video, AVPacket* packet, AVFrame* frame) {
	int response = avcodec_send_packet(video->audioCodecContext_I, packet);
	if (response < 0) {
		printf("[ERROR] Failed to send audio packet to decoder\n");
		return response;
	}
	while (response >= 0) {
		response = avcodec_receive_frame(video->audioCodecContext_I, frame);
		if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
			break;
		} else if (response < 0) {
			printf("[ERROR] Failed to receive audio frame from decoder\n");
			return response;
		}
		if (response >= 0) {
			// Do stuff and encode
			AVFrame* resampledFrame = av_frame_alloc();
			if (!resampledFrame) {
				printf("[ERROR] Failed to allocate memory for resampled frame\n");
				return -1;
			}
			av_frame_copy_props(resampledFrame, frame);
			resampledFrame->channel_layout = av_get_default_channel_layout(video->audioCodecContext_O->channels);
			resampledFrame->sample_rate = video->audioCodecContext_O->sample_rate;
			resampledFrame->format = AV_SAMPLE_FMT_S16;
			if (swr_convert_frame(video->swrContext, resampledFrame, frame) < 0) {
				printf("[ERROR] Failed to resample audio frame\n");
				return -1;
			}
			if (encodeAudio(video, resampledFrame) < 0) {
				printf("[ERROR] Failed to encode new audio\n");
				return -1;
			}
		}
		av_frame_unref(frame);
	}
	return 0;
}

int encodeAudio(Video* video, AVFrame* frame) {
	AVPacket* packet = av_packet_alloc();
	if (!packet) {
		printf("[ERROR] Could not allocate memory for audio output packet\n");
		return -1;
	}
	int response = avcodec_send_frame(video->audioCodecContext_O, frame);
	if (response < 0) {
		printf("[ERROR] Failed to send audio frame for encoding\n");
		return response;
	}
	while (response >= 0) {
		response = avcodec_receive_packet(video->audioCodecContext_O, packet);
		if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
			break;
		} else if (response < 0) {
			printf("[ERROR] Failed to receive audio packet from encoder\n");
			return response;
		}
		packet->stream_index = video->audioStream;
		video->inputStream = getAudioStream(video);
		video->outputStream = video->outputContext->streams[packet->stream_index];
		packet->pts = av_rescale_q_rnd(packet->pts, video->inputStream->time_base, video->outputStream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
		packet->dts = av_rescale_q_rnd(packet->dts, video->inputStream->time_base, video->outputStream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
		packet->duration = av_rescale_q(packet->duration, video->inputStream->time_base, video->outputStream->time_base);
		packet->pos = -1;

		response = av_interleaved_write_frame(video->outputContext, packet);
		if (response < 0) {
			printf("[ERROR] Failed to write audio packet\n");
			break;
		}
	}
	av_packet_unref(packet);
	av_packet_free(&packet);
	return 0;
}

int findPacket(AVFormatContext* inputContext, int frameIndex, int stream) {
	int64_t timebase;
	if (stream < 0) {
		timebase = AV_TIME_BASE;
	} else if (stream >= 0) {
		timebase = (inputContext->streams[stream]->time_base.den) / inputContext->streams[stream]->time_base.num;
	}
	int64_t seekTarget = timebase * frameIndex / VIDEO_DEFAULT_FPS;
	if (av_seek_frame(inputContext, stream, seekTarget, AVSEEK_FLAG_ANY) < 0) {
		printf("[ERROR] Failed to find keyframe from frame index %i\n", frameIndex);
		return -1;
	}
	return 0;
}

bool isFrameInteresting(Video* video, AVFrame* frame, int stream, double threshold) {
	if (stream == video->videoStream) {
		int width = video->videoCodecContext_I->width;
		int height = video->videoCodecContext_I->height;
		int r = 0;
		int g = 0;
		int b = 0;
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				int p = (x * 3) + (y * frame->linesize[0]);
				r += frame->data[0][p];
				g += frame->data[0][p + 1];
				b += frame->data[0][p + 2];
			}
		}
		int frameArea = height * width;
		r /= frameArea;
		b /= frameArea;
		g /= frameArea;
		int avgRGB = (r + g + b) / 3;
		double change = abs(avgRGB - video->lastRGBAvg) / (double)avgRGB;
		video->lastRGBAvg = avgRGB;
		if (change >= threshold) {
			return true;
		}
	} else if (stream == video->audioStream) {
		float* totalSample = (float*)malloc(2 * sizeof(float));
		for (int i = 0; i < frame->channels; i++) {
			for (int j = 0; j < frame->nb_samples; j++) {
				totalSample[i] += frame->data[i][j];
			}
		}
		totalSample[0] /= frame->nb_samples;
		totalSample[1] /= frame->nb_samples;
		float avgSample = (totalSample[0] + totalSample[1]) / 2;
		double change = abs(avgSample - video->lastAudioAvg) / avgSample;
		video->lastAudioAvg = avgSample;
		if (change >= threshold) {
			return true;
		}
	}
	return false;
}

void initSwsContext(int height, int width, uint8_t** buffer, struct SwsContext** swsContext) {
	int numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, width, height);
	*buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
	*swsContext = sws_getContext(width, height, AV_PIX_FMT_YUV420P, width, height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
}

AVFrame* getRGBFrame(Video* video, AVFrame* frame) {
	AVFrame* rgbFrame = av_frame_alloc();
	avpicture_fill((AVPicture*)rgbFrame, video->rgbBuffer, AV_PIX_FMT_RGB24, video->videoCodecContext_I->width, video->videoCodecContext_I->height);
	sws_scale(video->swsContext, (uint8_t const * const *)frame->data, frame->linesize, 0, video->videoCodecContext_I->height, rgbFrame->data, rgbFrame->linesize);
	return rgbFrame;
}

void populateFrameArray(Video* video, int frameIndex) {
	int startIndex = frameIndex - (VIDEO_DEFAULT_FPS * 3); // Starts 3 seconds before
	int endIndex = frameIndex + (VIDEO_DEFAULT_FPS * 3);	// Ends 3 seconds after
	if (startIndex < 0) {
		startIndex = 0;
	}
	if (video->clipIndex < video->numClips) {
		if (video->clipIndex > 0) {
			if (startIndex <= (video->frames)[video->clipIndex - 1][1]) {
				(video->frames)[video->clipIndex - 1][1] = endIndex;
				printf("Start: %i End: %i\n", (video->frames)[video->clipIndex - 1][0], (video->frames)[video->clipIndex - 1][1]);
				return;
			}
		}
		(video->frames)[video->clipIndex][0] = startIndex;
		(video->frames)[video->clipIndex][1] = endIndex;
		printf("Start: %i End: %i\n", (video->frames)[video->clipIndex][0], (video->frames)[video->clipIndex][1]);
		video->clipIndex += 1;
	}
}

void freeVideo(Video* video) {
	avcodec_close(video->videoCodecContext_I);
	avcodec_close(video->audioCodecContext_I);
	avcodec_close(video->videoCodecContext_O);
	avcodec_close(video->audioCodecContext_O);
	avformat_close_input(&(video->inputContext));
	avformat_close_input(&(video->outputContext));
	avformat_free_context(video->inputContext);
	avformat_free_context(video->outputContext);

	for (int i = 0; i < video->numClips; i++) {
		free((video->frames)[i]);
	}
	free(video->frames);
	free(video);
}
