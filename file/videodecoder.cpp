#include "videodecoder.h"

#pragma comment(lib,"avcodec.lib") 
#pragma comment(lib,"avdevice.lib") 
#pragma comment(lib,"avfilter.lib") 
#pragma comment(lib,"avformat.lib") 
#pragma comment(lib,"avutil.lib") 
#pragma comment(lib,"postproc.lib") 
#pragma comment(lib,"swresample.lib") 
#pragma comment(lib,"swscale.lib") 

VideoDecoder::VideoDecoder()
{
	mSleepTime = chtxprGetRtspSleepTime();
}

void VideoDecoder::startPlay(std::string _url) {
	this->videoName = _url;
	this->start();
}

void VideoDecoder::run() {
//解码器指针
	AVCodec *pCodec;
	//ffmpeg解码类的类成员
	AVCodecContext *pCodecCtx;
	//多媒体帧，保存解码后的数据帧
	AVFrame *pAvFrame;
	//保存视频流的信息
	AVFormatContext *pFormatCtx;

	//const char *filename = chtxprGetTestVideo().toStdString().c_str();
	const char *filename =this->videoName.c_str();
	//注册库中所有可用的文件格式和编码器
	av_register_all();

	pFormatCtx = avformat_alloc_context();
	//检查文件头部
	if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0) {
		printf("Can't find the stream!\n");
	}
	//查找流信息
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		printf("Can't find the stream information !\n");
	}

	int videoindex = -1;
	//遍历各个流，找到第一个视频流,并记录该流的编码信息
	for (int i = 0; i < pFormatCtx->nb_streams; ++i) {
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoindex = i;
			break;
		}
	}
	if (videoindex == -1) {
		printf("Don't find a video stream !\n");
		return ;
	}
	//得到一个指向视频流的上下文指针
	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	//到该格式的解码器
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL) {
		//寻找解码器
		printf("Cant't find the decoder !\n");
		return ;
	}
	//打开解码器
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		printf("Can't open the decoder !\n");
		return ;
	}

	//分配帧存储空间
	pAvFrame = av_frame_alloc();
	//存储解码后转换的RGB数据
	AVFrame *pFrameBGR = av_frame_alloc();

	// 保存BGR，opencv中是按BGR来保存的
	int size = avpicture_get_size(AV_PIX_FMT_BGR24, pCodecCtx->width,
		pCodecCtx->height);
	uint8_t *out_buffer = (uint8_t *)av_malloc(size);
	avpicture_fill((AVPicture *)pFrameBGR, out_buffer, AV_PIX_FMT_BGR24,
		pCodecCtx->width, pCodecCtx->height);

	AVPacket *packet = (AVPacket *)malloc(sizeof(AVPacket));
	printf("-----------输出文件信息---------\n");
	av_dump_format(pFormatCtx, 0, filename, 0);
	printf("------------------------------");

	struct SwsContext *img_convert_ctx;
	img_convert_ctx =
		sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
			pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL,
			NULL);

	int ret;
	int got_picture;

	while (1) {
		if (av_read_frame(pFormatCtx, packet) >= 0) {
			if (packet->stream_index == videoindex) {
				ret = avcodec_decode_video2(pCodecCtx, pAvFrame, &got_picture, packet);
				if (ret < 0) {
					printf("Decode Error.（解码错误）\n");
					return ;
				}
				if (got_picture) {
					printf("got a picture\n");
					//YUV to RGB
					sws_scale(img_convert_ctx, (const uint8_t *const *)pAvFrame->data,
						pAvFrame->linesize, 0, pCodecCtx->height, pFrameBGR->data, pFrameBGR->linesize);

					cv::Mat* pCvMat=new cv::Mat(cv::Size(pCodecCtx->width, pCodecCtx->height), CV_8UC3);
					memcpy(pCvMat->data, out_buffer, size);
					frames.push_back(pCvMat);
					emit sigSendFrame(pCvMat);
					frames.erase(std::remove_if(frames.begin(), frames.end(), [&](const cv::Mat *p) {
						return p->empty(); }), frames.end());
				}
			}
			av_free_packet(packet);
		}
		else {
			break;
		}

		this->msleep(mSleepTime);
	}

	av_free(out_buffer);
	av_free(pFrameBGR);
	av_free(pAvFrame);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	sws_freeContext(img_convert_ctx);
}

VideoDecoder::~VideoDecoder()
{
}
