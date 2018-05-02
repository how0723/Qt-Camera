#include "rtspstream.h"

#pragma comment(lib,"avcodec.lib") 
#pragma comment(lib,"avdevice.lib") 
#pragma comment(lib,"avfilter.lib") 
#pragma comment(lib,"avformat.lib") 
#pragma comment(lib,"avutil.lib") 
#pragma comment(lib,"postproc.lib") 
#pragma comment(lib,"swresample.lib") 
#pragma comment(lib,"swscale.lib") 

RtspStream::RtspStream()
{
}

void RtspStream::startPlay(std::string _url) {
	this->videoName = _url;
	this->start();
}

void RtspStream::run() {
	AVFormatContext *pFormatCtx;
	AVCodecContext *pCodecCtx;
	AVCodec *pCodec;
	AVFrame *pFrame, *pFrameBGR;
	AVPacket *packet;
	uint8_t *out_buffer;

	static struct SwsContext *img_convert_ctx;

	int videoStream, i, numBytes;
	int ret, got_picture;

	avformat_network_init();   ///初始化FFmpeg网络模块，2017.8.5---lizhen
	av_register_all();         //初始化FFMPEG  调用了这个才能正常适用编码器和解码器

							   //Allocate an AVFormatContext.
	pFormatCtx = avformat_alloc_context();

	AVDictionary *avdic = NULL;
	char option_key[] = "rtsp_transport";
	char option_value[] = "tcp";
	av_dict_set(&avdic, option_key, option_value, 0);
	char option_key2[] = "max_delay";
	char option_value2[] = "200";
	av_dict_set(&avdic, option_key2, option_value2, 0);
	//rtsp地址
	//char url[] = "rtsp://admin:pnwg9647@192.168.1.64:554/h264/ch1/main/av_stream";
	//char url[] = "./test.MOV";

	if (avformat_open_input(&pFormatCtx, this->videoName.c_str(), NULL, &avdic) != 0) {
		printf("can't open the file. \n");
		return;
	}

	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		printf("Could't find stream infomation.\n");
		return;
	}

	videoStream = -1;

	///循环查找视频中包含的流信息，直到找到视频类型的流
	///便将其记录下来 保存到videoStream变量中
	for (i = 0; i < pFormatCtx->nb_streams; i++) {
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStream = i;
		}
	}

	///如果videoStream为-1 说明没有找到视频流
	if (videoStream == -1) {
		printf("Didn't find a video stream.\n");
		return;
	}

	///查找解码器
	pCodecCtx = pFormatCtx->streams[videoStream]->codec;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	///2017.8.9---lizhen
	pCodecCtx->bit_rate = 0;   //初始化为0
	pCodecCtx->time_base.num = 1;  //下面两行：一秒钟20帧
	pCodecCtx->time_base.den = 20;
	pCodecCtx->frame_number = 1;  //每包一个视频帧

	if (pCodec == NULL) {
		printf("Codec not found.\n");
		return;
	}

	///打开解码器
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		printf("Could not open codec.\n");
		return;
	}

	pFrame = av_frame_alloc();
	pFrameBGR = av_frame_alloc();

	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
		pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
		AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);

	numBytes = avpicture_get_size(AV_PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height);

	out_buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
	avpicture_fill((AVPicture *)pFrameBGR, out_buffer, AV_PIX_FMT_BGR24,
		pCodecCtx->width, pCodecCtx->height);

	int y_size = pCodecCtx->width * pCodecCtx->height;

	packet = (AVPacket *)malloc(sizeof(AVPacket)); //分配一个packet
	av_new_packet(packet, y_size); //分配packet的数据

	while (1)
	{
		if (av_read_frame(pFormatCtx, packet) < 0) break; //这里认为视频读取完了
		
		if (packet->stream_index == videoStream) {
			ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);

			if (ret < 0) {
				printf("decode error.\n");
				return;
			}

			if (got_picture) {
				sws_scale(img_convert_ctx, (uint8_t const * const *)pFrame->data,
					pFrame->linesize, 0, pCodecCtx->height, pFrameBGR->data,
					pFrameBGR->linesize);

				cv::Mat *pCvMat =
					new cv::Mat(cv::Size(pCodecCtx->width, pCodecCtx->height), CV_8UC3);
				memcpy(pCvMat->data, out_buffer, numBytes);
				frames.push_back(pCvMat);
				emit sigSendFrame(pCvMat);
				frames.erase(std::remove_if(frames.begin(), frames.end(), [&](const cv::Mat *p) {
					return p->empty(); }), frames.end());
			}
		}
		av_free_packet(packet); //释放资源,否则内存会一直上升
	}
	av_free(out_buffer);
	av_free(pFrameBGR);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);
}

RtspStream::~RtspStream()
{
}
