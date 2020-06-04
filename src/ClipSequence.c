#include "ClipSequence.h"

void initSequence(ClipSequence** sequence) {
	if (!*sequence) {
		*sequence = (ClipSequence*)malloc(sizeof(ClipSequence));
	}
	(*sequence)->videos = (VideoList*)malloc(sizeof(VideoList));
	(*sequence)->v_firstdts = 0;
	(*sequence)->a_firstdts = 0;
	(*sequence)->v_lastdts = 0;
	(*sequence)->a_lastdts = 0;
}

int addVideo(ClipSequence* sequence, Video* video) {
	if (video == NULL) {
		printf("[NULL] Input video does not exist\n");
		return -1;
	}
	insert(sequence->videos, video);
}

int openCurrentVideo(ClipSequence* sequence, Video* video/*, char* outputFile*/) {
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
	printf("[OPEN] Video %s opened\n", video->filename);
	return 0;
}

int allocateSequenceOutput(ClipSequence* sequence, char* outputFile) {
	avformat_alloc_output_context2(&(sequence->outputContext), NULL, NULL, outputFile);
	if (!sequence->outputContext) {
		printf("[ERROR] Failed to create output context\n");
		return -1;
	}
	if (!(sequence->outputContext->oformat->flags & AVFMT_NOFILE)) {
    if (avio_open(&(sequence->outputContext->pb), outputFile, AVIO_FLAG_WRITE) < 0) {
      printf("Could not open output file %s", outputFile);
      return -1;
    }
  }
	return 0;
}

int findVideoStreams(ClipSequence* sequence, Video* video) {
	for (int i = 0; i < video->inputContext->nb_streams; i++) {
		video->inputStream = video->inputContext->streams[i];
		if (video->inputContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			video->videoStream = i;
			if (prepareStreamInfo(&(video->videoCodecContext_I), &(video->videoCodec), video->inputStream) < 0) {
				printf("[ERROR] Could not prepare video stream information\n");
				return -1;
			}
		} else if (video->inputContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			video->audioStream = i;
			if (prepareStreamInfo(&(video->audioCodecContext_I), &(video->audioCodec), video->inputStream) < 0) {
				printf("[ERROR] Could not prepare audio stream information\n");
				return -1;
			}
		}
		sequence->outputStream = avformat_new_stream(sequence->outputContext, NULL);
		if (!sequence->outputStream) {
			printf("[ERROR] Failed allocating output stream\n");
			return -1;
		}
		if (avcodec_parameters_copy(sequence->outputStream->codecpar, video->inputStream->codecpar) < 0) {
			printf("[ERROR] Failed to copy video codec parameters to sequence output stream\n");
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
	return 0;
}

int copySequenceFrames(ClipSequence* sequence, Video* video, AVPacket* packet, AVFrame* frame) {
	if (!packet) {
		printf("[ERROR] Packet not allocated to be read\n");
		return -1;
	}
	if (!frame) {
		printf("[ERROR] Frame not allocated to be read\n");
		return -1;
	}
	while (av_read_frame(video->inputContext, packet) >= 0) {
		if (packet->stream_index == video->videoStream) {
			if (decodeVideoSequence(sequence, video, packet, frame) < 0) {
				printf("[ERROR] Failed to decode and encode video\n");
				return -1;
			}
		} else if (packet->stream_index == video->audioStream) {
			if (decodeAudioSequence(sequence, video, packet, frame) < 0) {
				printf("[ERROR] Failed to decode and encode audio\n");
				return -1;
			}
		}
		av_packet_unref(packet);
	}
	sequence->v_lastdts += sequence->v_currentdts;
	sequence->a_lastdts += sequence->a_currentdts;
	// Flush encoder
	//encodeVideoSequence(sequence, video, NULL);
	//encodeAudioSequence(sequence, video, NULL);
	return 0;
}

int decodeVideoSequence(ClipSequence* sequence, Video* video, AVPacket* packet, AVFrame* frame) {
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
			sequence->v_currentdts = packet->dts - sequence->v_firstdts;
			if (encodeVideoSequence(sequence, video, frame) < 0) {
				printf("[ERROR] Failed to encode new video\n");
				return -1;
			}
		}
		av_frame_unref(frame);
	}
	return 0;
}

int encodeVideoSequence(ClipSequence* sequence, Video* video, AVFrame* frame) {
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
		packet->duration = VIDEO_PACKET_DURATION;
		int64_t cts = packet->pts - packet->dts;
		packet->dts = sequence->v_currentdts + sequence->v_lastdts + packet->duration;
		packet->pts = packet->dts + cts;
		packet->stream_index = video->videoStream;
		response = av_interleaved_write_frame(sequence->outputContext, packet);
		if (response < 0) {
			printf("[ERROR] Failed to write video packet\n");
			break;
		}
	}
	av_packet_unref(packet);
	av_packet_free(&packet);
	return 0;
}

int decodeAudioSequence(ClipSequence* sequence, Video* video, AVPacket* packet, AVFrame* frame) {
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
			sequence->a_currentdts = packet->dts - sequence->a_firstdts;

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
			if (encodeAudioSequence(sequence, video, resampledFrame) < 0) {
				printf("[ERROR] Failed to encode new audio\n");
				return -1;
			}
		}
		av_frame_unref(frame);
	}
	return 0;
}

int encodeAudioSequence(ClipSequence* sequence, Video* video, AVFrame* frame) {
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
		packet->duration = VIDEO_PACKET_DURATION;
		int64_t cts = packet->pts - packet->dts;
		packet->dts = sequence->a_currentdts + sequence->a_lastdts + packet->duration;
		packet->pts = packet->dts + cts;
		packet->stream_index = video->audioStream;
		response = av_interleaved_write_frame(sequence->outputContext, packet);
		if (response < 0) {
			printf("[ERROR] Failed to write audio packet\n");
			break;
		}
	}
	av_packet_unref(packet);
	av_packet_free(&packet);
	return 0;
}

int cutVideo(ClipSequence* sequence, Video* video, int startFrame, int endFrame) {
	if (findPacket(video->inputContext, startFrame, 0) < 0) {
		printf("[ERROR] Failed to find packet\n");
	}
	AVPacket* packet = av_packet_alloc();
	if (!packet) {
		printf("[ERROR] Could not allocate packet for cutting video\n");
		return -1;
	}
	AVFrame* frame = av_frame_alloc();
	if (!frame) {
		printf("[ERROR] Could not allocate frame for cutting video\n");
		return -1;
	}
	int currentFrame = startFrame;
	bool v_firstframe = true;
	bool a_firstframe = true;
	while (av_read_frame(video->inputContext, packet) >= 0 && currentFrame <= endFrame) {
		if (packet->stream_index == video->videoStream) {
			// Only count video frames since seeking is based on 60 fps video frames
			currentFrame++;
			if (v_firstframe) {
				v_firstframe = false;
				sequence->v_firstdts = packet->dts;
			}
			if (decodeVideoSequence(sequence, video, packet, frame) < 0) {
				printf("[ERROR] Failed to decode and encode video\n");
				return -1;
			}
		} else if (packet->stream_index == video->audioStream) {
			if (a_firstframe) {
				a_firstframe = false;
				sequence->a_firstdts = packet->dts;
			}
			if (decodeAudioSequence(sequence, video, packet, frame) < 0) {
				printf("[ERROR] Failed to decode and encode audio\n");
				return -1;
			}
		}
		av_packet_unref(packet);
	}
	sequence->v_lastdts += sequence->v_currentdts;
	sequence->a_lastdts += sequence->a_currentdts;
	return 0;
}

void freeSequence(ClipSequence* sequence) {
	avformat_close_input(&(sequence->outputContext));
	avformat_free_context(sequence->outputContext);
	free(sequence);
}
