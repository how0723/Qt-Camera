#pragma once

#include <QObject>
#include "common/common.h"

class RTSPCam : public QThread
{
	Q_OBJECT

public:
	RTSPCam();
	~RTSPCam();
	void startPlay(std::string url);
signals:
	void sigSendFrame(cv::Mat* frame);
protected:
	void run();
private:
	cv::VideoCapture cap;
	cv::Mat frame;
	std::vector<cv::Mat*> frames;
	int mSleepTime = 28;
};
