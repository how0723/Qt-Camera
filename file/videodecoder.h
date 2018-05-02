#pragma once

#include <QObject>
#include "common/common.h"

extern "C" {
#include "libavutil/avutil.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}; 


class VideoDecoder : public QThread
{
	Q_OBJECT

public:
	VideoDecoder();
	~VideoDecoder();
	void startPlay(std::string _url);
signals:
	void sigSendFrame(cv::Mat* frame);
protected:
	void run();
private:
	std::string videoName = "";
	std::vector<cv::Mat*> frames;
	int mSleepTime = 28;

};
