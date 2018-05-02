#pragma once

#include <QObject>
#include "common/common.h"

extern "C" {
#include "libavutil/avutil.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}; 


class RtspStream : public QThread
{
	Q_OBJECT

public:
	RtspStream();
	~RtspStream();
	void startPlay(std::string _url);
signals:
	void sigSendFrame(cv::Mat* frame);
protected:
	void run();
private:
	std::string videoName = "";
	std::vector<cv::Mat*> frames;
};
