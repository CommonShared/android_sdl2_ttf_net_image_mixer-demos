// SDL_FFMPEG_DEMO.cpp : Defines the entry point for the console application.
// The lib version:
// SDL 2.0.4 avcodec-57 avdevice-57 avfilter-6 avformat-57 avutil-55 postproc-54 swresample-2 swscale-4

#include <iostream>
#include <stdio.h>
#include <fstream>
#include <string>
#include <queue>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <SDL.h>
//#undef main
#include <SDL_thread.h>
}

#define IS_FFMPEG_VERSION_NEW		0x00

#define MAX_AUDIO_NUM				128
#define MAX_VIDEO_NUM				128
#define SDL_AUDIO_BUFFER_SIZE		1024
#define MAX_AUDIO_FRAME_SIZE		192000

// compatibility with newer API
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame
#endif

SDL_Window* window = nullptr;
SDL_Renderer* renderer;
SDL_Texture* texture = nullptr;
struct SwsContext* sws_ctx = nullptr;

int play_on_sdl(AVCodecContext* pCodecCtx, AVFrame* frame) {

	if (window == nullptr)
	{
		window = SDL_CreateWindow("MyVideoWindow",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			pCodecCtx->width,
			pCodecCtx->height,
			SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

		texture = SDL_CreateTexture(renderer,
			SDL_PIXELFORMAT_YV12,
			//SDL_TEXTUREACCESS_TARGET,
			SDL_TEXTUREACCESS_STREAMING,
			pCodecCtx->width,
			pCodecCtx->height);
		if (texture == nullptr)
		{
			SDL_DestroyWindow(window);
			window = nullptr;
		}
		else
		{
			sws_ctx = sws_getContext(pCodecCtx->width,
				pCodecCtx->height,
				pCodecCtx->pix_fmt,
				pCodecCtx->width,
				pCodecCtx->height,
				AV_PIX_FMT_YUV420P,
				SWS_BILINEAR,
				NULL,
				NULL,
				NULL
			);
			if (sws_ctx == nullptr)
			{
				std::cout << "SDL: could not set video mode - exiting\n" << std::endl;
				//exit(1);
			}
		}
	}

	AVFrame* pFrameYUV = av_frame_alloc();
	int numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width,
		pCodecCtx->height, 1);
	uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));

	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, buffer, AV_PIX_FMT_YUV420P,
		pCodecCtx->width, pCodecCtx->height, 1);

	sws_scale(sws_ctx, frame->data,
		frame->linesize, 0, pCodecCtx->height,
		pFrameYUV->data, pFrameYUV->linesize);

	SDL_UpdateYUVTexture(texture,
		nullptr,
		pFrameYUV->data[0],
		pFrameYUV->linesize[0],
		pFrameYUV->data[1],
		pFrameYUV->linesize[1],
		pFrameYUV->data[2],
		pFrameYUV->linesize[2]);

	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, nullptr, nullptr);
	SDL_RenderPresent(renderer);

	return 0;
}

SDL_Event event = { 0 };
bool quit = false;
bool play = false;

void handle_input() {

	if (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_KEYUP:
			switch (event.key.keysym.sym) {
			case SDLK_ESCAPE:
				quit = true;
				std::cout << "exit" << std::endl;
				break;
			case SDLK_SPACE:
				play = !play;
				std::cout << "play" << std::endl;
				break;
			default:
				break;
			}
			break;
		case SDL_QUIT:
			quit = true;
			break;
		default:
			break;
		}
	}
}

typedef struct PacketQueue
{
	std::queue<AVPacket> queue;

	int nb_packets;
	int size;

	SDL_mutex* mutex;
	SDL_cond* cond;

	PacketQueue()
	{
		nb_packets = 0;
		size = 0;

		mutex = SDL_CreateMutex();
		cond = SDL_CreateCond();
	}

	bool enQueue(const AVPacket* packet)
	{
		AVPacket pkt;
		if (av_packet_ref(&pkt, packet) < 0)
		{
			return false;
		}

		SDL_LockMutex(mutex);
		queue.push(pkt);

		size += pkt.size;
		nb_packets++;

		SDL_CondSignal(cond);
		SDL_UnlockMutex(mutex);

		return true;
	}

	bool deQueue(AVPacket* packet, bool block)
	{
		bool ret = false;

		SDL_LockMutex(mutex);
		while (true)
		{
			if (quit)
			{
				ret = false;
				break;
			}

			if (!queue.empty())
			{
				if (av_packet_ref(packet, &queue.front()) < 0)
				{
					ret = false;
					break;
				}
				queue.pop();
				nb_packets--;
				size -= packet->size;

				ret = true;
				break;
			}
			else if (!block)
			{
				ret = false;
				break;
			}
			else
			{
				SDL_CondWait(cond, mutex);
			}
		}
		SDL_UnlockMutex(mutex);
		return ret;
	}
}PacketQueue;

PacketQueue audioq;

// 从Packet中解码，返回解码的数据长度
int audio_decode_frame(AVCodecContext* aCodecCtx, uint8_t* audio_buf, int buf_size)
{
	AVFrame* frame = av_frame_alloc();
	int data_size = 0;
	AVPacket pkt;

	SwrContext* swr_ctx = nullptr;

	if (quit)
	{
		return -1;
	}
	if (!audioq.deQueue(&pkt, true))
	{
		return -1;
	}
	int ret = avcodec_send_packet(aCodecCtx, &pkt);
	if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
	{
		return -1;
	}
	ret = avcodec_receive_frame(aCodecCtx, frame);
	if (ret < 0 && ret != AVERROR_EOF)
	{
		return -1;
	}
	int index = av_get_channel_layout_channel_index(av_get_default_channel_layout(4), AV_CH_FRONT_CENTER);

	// 设置通道数或channel_layout
	if (frame->channels > 0 && frame->channel_layout == 0)
	{
		frame->channel_layout = av_get_default_channel_layout(frame->channels);
	}
	else if (frame->channels == 0 && frame->channel_layout > 0)
	{
		frame->channels = av_get_channel_layout_nb_channels(frame->channel_layout);
	}
	AVSampleFormat dst_format = av_get_packed_sample_fmt((AVSampleFormat)frame->format); //AV_SAMPLE_FMT_S16;//
	Uint64 dst_layout = av_get_default_channel_layout(frame->channels);
	// 设置转换参数
	swr_ctx = swr_alloc_set_opts(nullptr, dst_layout, dst_format, frame->sample_rate,
		frame->channel_layout, (AVSampleFormat)frame->format, frame->sample_rate, 0, nullptr);
	if (!swr_ctx || swr_init(swr_ctx) < 0)
	{
		return -1;
	}
	// 计算转换后的sample个数 a * b / c
	int dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, frame->sample_rate) + frame->nb_samples, frame->sample_rate, frame->sample_rate, AVRounding(1));
	// 转换，返回值为转换后的sample个数
	int nb = swr_convert(swr_ctx, &audio_buf, dst_nb_samples, (const uint8_t**)frame->data, frame->nb_samples);
	data_size = frame->channels * nb * av_get_bytes_per_sample(dst_format);

	av_frame_free(&frame);
	swr_free(&swr_ctx);
	return data_size;
}

void audio_callback(void* userdata, Uint8* stream, int len)
{
	AVCodecContext* aCodecCtx = (AVCodecContext*)userdata;

	int len1, audio_size;

	static uint8_t audio_buff[(MAX_AUDIO_FRAME_SIZE * 3) / 2] = { 0 };
	static unsigned int audio_buf_size = 0;
	static unsigned int audio_buf_index = 0;

	SDL_memset(stream, 0, len);

	while (len > 0)// 想设备发送长度为len的数据
	{
		if (audio_buf_index >= audio_buf_size) // 缓冲区中无数据
		{
			// 从packet中解码数据
			audio_size = audio_decode_frame(aCodecCtx, audio_buff, sizeof(audio_buff));
			if (audio_size < 0) // 没有解码到数据或出错，填充0
			{
				audio_buf_size = 0;
				memset(audio_buff, 0, audio_buf_size);
			}
			else
			{
				audio_buf_size = audio_size;
			}
			audio_buf_index = 0;
		}
		len1 = audio_buf_size - audio_buf_index; // 缓冲区中剩下的数据长度
		if (len1 > len) // 向设备发送的数据长度为len
		{
			len1 = len;
		}

		memcpy(stream, audio_buff + audio_buf_index, len);
		//SDL_MixAudio(stream, audio_buff + audio_buf_index, len, SDL_MIX_MAXVOLUME);

		len -= len1;
		stream += len1;
		audio_buf_index += len1;
	}
}

#include <android/log.h>

#define LOG_TAG    "JNILOG"
#undef LOG
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define LOGF(...)  __android_log_print(ANDROID_LOG_FATAL,LOG_TAG,__VA_ARGS__)

int main(int argc, char** argv)
{
	AVFormatContext* pFormatCtx = nullptr;
	int             nIndex = 0;
	int nAudioCount = 0;
	int	nAudioStream[MAX_AUDIO_NUM] = { 0 };
	int nVideoCount = 0;
	int	nVideoStream[MAX_VIDEO_NUM] = { 0 };
	AVCodecContext* pCodecCtxAudio = nullptr;
	AVCodecContext* pCodecCtxAudio_duplicate = nullptr;
	AVCodecContext* pCodecCtxVideo = nullptr;
	AVCodec* pVCodec = nullptr;
	AVCodec* pACodec = nullptr;
	AVFrame* pFrame = nullptr;
	AVFrame* pFrameRGB = nullptr;
	AVPacket          packet = { 0 };
	int               frameFinished = 0;
	int               numBytes = 0;
	uint8_t* buffer = nullptr;
	struct SwsContext* sws_ctx = nullptr;

	SDL_AudioSpec wanted_spec = { 0 };
	SDL_AudioSpec spec = { 0 };
	AVDictionary* videoOptionsDict = nullptr;
	AVDictionary* audioOptionsDict = nullptr;

	memset(nAudioStream, 0xFF, sizeof(nAudioStream));
	memset(nVideoStream, 0xFF, sizeof(nVideoStream));
	LOGE("enter main");
#if IS_FFMPEG_VERSION_NEW//对应当前FFMPEG版本
	//av_register_input_format();
	//av_register_output_format();
#else
	// Register all formats and codecs
	av_register_all();
#endif

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		std::cout << "could not init sdl" << SDL_GetError() << std::endl;

		LOGE( "could not init sdl(%s)", SDL_GetError());
		return (-1);
	}

	// Open video file
	//const char* filename = "trailer.mp4";// "C://Users//bill_lv//Desktop//抽象编程//VID_20160127_162206.mp4";
	//const char* filename = "oceans.mp4";
	const char* filename = "big_buck_bunny.mp4";
	//const char* filename = "Wildlife.wmv";
	//const char* filename = "棋王.King.of.Chess.1991.HK.DVDRip.x264.2Audio.AC3.iNT-NowYS.mkv";
	if (avformat_open_input(&pFormatCtx, filename, nullptr, nullptr) != 0)
	{
		LOGE( "could not avformat_open_input(%s,%s)", filename,SDL_GetError());
		return -1; // Couldn't open file
	}
	// Retrieve stream information
	if (avformat_find_stream_info(pFormatCtx, nullptr) < 0)
	{
		LOGE( "could not avformat_find_stream_info(%s)", filename);
		// Close the video file
		avformat_close_input(&pFormatCtx);
		return -1; // Couldn't find stream information
	}

	av_dump_format(pFormatCtx, 0, filename, 0);

	// Find the first audio and video stream
	for (nIndex = 0; nIndex < pFormatCtx->nb_streams; nIndex++)
	{
#if IS_FFMPEG_VERSION_NEW//对应当前FFMPEG版本
		if (pFormatCtx->streams[nIndex]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
#else//旧版本
		if (pFormatCtx->streams[nIndex]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
#endif
		{
			nVideoStream[nVideoCount++] = nIndex;
		}
#if IS_FFMPEG_VERSION_NEW//对应当前FFMPEG版本
		if (pFormatCtx->streams[nIndex]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
#else//旧版本
		if (pFormatCtx->streams[nIndex]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
#endif
		{
			nAudioStream[nAudioCount++] = nIndex;
		}
	}
	if (nVideoCount == 0)
	{
		LOGE( "could not nVideoCount=0(%s)", filename);
		// Close the video file
		avformat_close_input(&pFormatCtx);
		return -1; // Didn't find a video stream
	}

	/*if (nAudioCount != 0)
	{
		// Get a pointer to the codec context for the audio stream
#if IS_FFMPEG_VERSION_NEW//对应当前FFMPEG版本
		pCodecCtxAudio = avcodec_alloc_context3(nullptr);
		if (pCodecCtxAudio == NULL)
		{
			fprintf(stderr, "Could not allocate AVCodecContext\n");
			avformat_close_input(&pFormatCtx);
			return -1;
		}
		avcodec_parameters_to_context(pCodecCtxAudio, pFormatCtx->streams[nAudioStream[0]]->codecpar);
#else//旧版本
		pCodecCtxAudio = pFormatCtx->streams[nAudioStream[0]]->codec;
#endif
		// Find the decoder for the audio stream
		pACodec = avcodec_find_decoder(pCodecCtxAudio->codec_id);
		if (pACodec == nullptr)
		{
			fprintf(stderr, "Unsupported codec!\n");
			avcodec_free_context(&pCodecCtxAudio);
			avformat_close_input(&pFormatCtx);
			return -1; // Codec not found
		}*/

		/*
	 // Copy context
#if IS_FFMPEG_VERSION_NEW//对应当前FFMPEG版本
		AVCodecParameters* avcodecParam = avcodec_parameters_alloc();
		if (avcodecParam == nullptr)
		{
			fprintf(stderr, "avcodec_parameters_alloc failed!error:%s\n", strerror(errno));
			avcodec_free_context(&pCodecCtxVideo);
			return -1; // Alloc failed
		}

		if (avcodec_parameters_from_context(avcodecParam, pCodecCtxAudio) < 0)
		{
			avcodec_parameters_free(&avcodecParam);
			avcodec_free_context(&pCodecCtxAudio);
			return -1; // Copy from failed
		}
		pCodecCtxAudio_duplicate = avcodec_alloc_context3(pACodec);
		if (avcodec_parameters_to_context(pCodecCtxAudio_duplicate, avcodecParam) < 0)
		{
			avcodec_parameters_free(&avcodecParam);
			avcodec_free_context(&pCodecCtxAudio);
			avcodec_free_context(&pCodecCtxAudio_duplicate);
			return -1; // Copy to failed
		}
		avcodec_parameters_free(&avcodecParam);
#else
		if (avcodec_copy_context(pCodecCtxAudio_duplicate, pCodecCtxAudio) != 0)
		{
			fprintf(stderr, "Couldn't copy codec context");
			return -1; // Error copying codec context
		}
#endif*/
// Set audio settings from codec info.
		/*wanted_spec.freq = pCodecCtxAudio->sample_rate;
		wanted_spec.format = AUDIO_S16SYS;
		wanted_spec.channels = pCodecCtxAudio->channels;
		wanted_spec.silence = 0;
		wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
		wanted_spec.callback = audio_callback;
		wanted_spec.userdata = pCodecCtxAudio;

		if (SDL_OpenAudio(&wanted_spec, &spec) == (-1))
		{
			fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
			avcodec_free_context(&pCodecCtxAudio);
			avformat_close_input(&pFormatCtx);
			return -1;
		}
		avcodec_open2(pCodecCtxAudio, pACodec, &audioOptionsDict);
		SDL_PauseAudio(0);
	}*/

	// Get a pointer to the codec context for the video stream
#if IS_FFMPEG_VERSION_NEW//对应当前FFMPEG版本
	pCodecCtxVideo = avcodec_alloc_context3(nullptr);
	if (pCodecCtxVideo == NULL)
	{
		fprintf(stderr, "Could not allocate AVCodecContext\n");
		LOGE( "Could not allocate AVCodecContext(%s)", SDL_GetError());
		avcodec_free_context(&pCodecCtxAudio);
		avformat_close_input(&pFormatCtx);
		return -1;
	}
	avcodec_parameters_to_context(pCodecCtxVideo, pFormatCtx->streams[nVideoStream[0]]->codecpar);
#else//旧版本
	pCodecCtxVideo = pFormatCtx->streams[nVideoStream[0]]->codec;
#endif

	// Find the decoder for the video stream
	pVCodec = avcodec_find_decoder(pCodecCtxVideo->codec_id);
	if (pVCodec == nullptr)
	{
		LOGE( "Could not find supported codec(%s)", SDL_GetError());
		fprintf(stderr, "Unsupported codec!\n");
		avcodec_free_context(&pCodecCtxVideo);
		avcodec_free_context(&pCodecCtxAudio);
		avformat_close_input(&pFormatCtx);
		return -1; // Codec not found
	}
	/*// Copy context
	pCodecCtx = avcodec_alloc_context3(pCodec);

#if IS_FFMPEG_VERSION_NEW//对应当前FFMPEG版本
	AVCodecParameters* avcodecParam = avcodec_parameters_alloc();
	if (avcodecParam == nullptr)
	{
		fprintf(stderr, "avcodec_parameters_alloc failed!error:%s\n", strerror(errno));
		avcodec_free_context(&pCodecCtxVideo);
		return -1; // Alloc failed
	}

	if (avcodec_parameters_from_context(avcodecParam, pCodecCtxVideo) < 0)
	{
		avcodec_parameters_free(&avcodecParam);
		avcodec_free_context(&pCodecCtxVideo);
		return -1; // Copy from failed
	}
	if (avcodec_parameters_to_context(pCodecCtx, avcodecParam) < 0)
	{
		avcodec_parameters_free(&avcodecParam);
		avcodec_free_context(&pCodecCtxVideo);
		return -1; // Copy to failed
	}
	avcodec_parameters_free(&avcodecParam);
#else
	if (avcodec_copy_context(pCodecCtx, pCodecCtxApp) != 0)
	{
		fprintf(stderr, "Couldn't copy codec context");
		return -1; // Error copying codec context
	}
#endif*/

// Open codec
	if (avcodec_open2(pCodecCtxVideo, pVCodec, &videoOptionsDict) < 0)
	{
		avcodec_free_context(&pCodecCtxVideo);
		avcodec_free_context(&pCodecCtxAudio);
		avformat_close_input(&pFormatCtx);
		return -1; // Could not open codec
	}

	// Allocate video frame
	pFrame = av_frame_alloc();
	if (pFrame == nullptr)
	{
		avcodec_free_context(&pCodecCtxVideo);
		avcodec_free_context(&pCodecCtxAudio);
		avformat_close_input(&pFormatCtx);
		return -1;
	}

	// Allocate an AVFrame structure
	pFrameRGB = av_frame_alloc();
	if (pFrameRGB == nullptr)
	{
		av_frame_free(&pFrame);
		avcodec_free_context(&pCodecCtxVideo);
		avcodec_free_context(&pCodecCtxAudio);
		avformat_close_input(&pFormatCtx);
		return -1;
	}

	// Determine required buffer size and allocate buffer
	numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtxVideo->width, pCodecCtxVideo->height, 1);
	buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
	if (buffer == nullptr)
	{
		av_frame_free(&pFrame);
		avcodec_free_context(&pCodecCtxVideo);
		avcodec_free_context(&pCodecCtxAudio);
		avformat_close_input(&pFormatCtx);
		return -1;
	}
	// Assign appropriate parts of buffer to image planes in pFrameRGB
	// Note that pFrameRGB is an AVFrame, but AVFrame is a superset
	// of AVPicture
	av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, AV_PIX_FMT_RGB24, pCodecCtxVideo->width, pCodecCtxVideo->height, 1);

	// initialize SWS context for software scaling
	sws_ctx = sws_getContext(pCodecCtxVideo->width,
		pCodecCtxVideo->height,
		pCodecCtxVideo->pix_fmt,
		pCodecCtxVideo->width,
		pCodecCtxVideo->height,
		AV_PIX_FMT_RGB24,
		SWS_BILINEAR,
		nullptr,
		nullptr,
		nullptr
	);
	LOGE( "ok");
	// Read frames andplay
	while (av_read_frame(pFormatCtx, &packet) >= 0)
	{
		handle_input();
		if (packet.stream_index == nAudioStream[0])
		{
			audioq.enQueue(&packet);
		}
		// Is this a packet from the video stream?
		else if (packet.stream_index == nVideoStream[0])
		{
#if IS_FFMPEG_VERSION_NEW//对应当前FFMPEG版本
			frameFinished = avcodec_send_packet(pCodecCtxVideo, &packet);
			if (frameFinished != 0)
			{
				continue;
			}
			frameFinished = avcodec_receive_frame(pCodecCtxVideo, pFrame);
			if (frameFinished != 0)
			{
				continue;
			}
			frameFinished = 1;
#else
			// Decode video frame
			avcodec_decode_video2(pCodecCtxVideo, pFrame, &frameFinished, &packet);
#endif

			// Did we get a video frame?
			if (frameFinished)
			{
				// Convert the image from its native format to RGB
				sws_scale(sws_ctx, pFrame->data,
					pFrame->linesize, 0, pCodecCtxVideo->height,
					pFrameRGB->data, pFrameRGB->linesize);

				play_on_sdl(pCodecCtxVideo, pFrame);

				//SDL_Delay(40);
			}
		}
		//else
		{
			// Free the packet that was allocated by av_read_frame
			av_packet_unref(&packet);
		}
	}

	// Free the RGB image
	av_free(buffer);
	av_frame_free(&pFrameRGB);

	// Free the YUV frame
	av_frame_free(&pFrame);

	// Close the codecs
	avcodec_close(pCodecCtxVideo);

	// Free the context frame
#if IS_FFMPEG_VERSION_NEW//对应当前FFMPEG版本
	avcodec_free_context(&pCodecCtxVideo);
#else
#endif

	// Close the video file
	avformat_close_input(&pFormatCtx);

	getchar();

	return 0;
}