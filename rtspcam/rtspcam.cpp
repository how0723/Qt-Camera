#include "rtspcam.h"
#ifndef QT_DEBUG
#pragma comment(lib,"IlmImf.lib")   
#pragma comment(lib,"ippicvmt.lib")  
#pragma comment(lib,"ippiw.lib")   
#pragma comment(lib,"ittnotify.lib")   
#pragma comment(lib,"libjasper.lib")   
#pragma comment(lib,"libjpeg.lib")   
#pragma comment(lib,"libpng.lib")   
#pragma comment(lib,"libtiff.lib")   
#pragma comment(lib,"libwebp.lib")   
#pragma comment(lib,"zlib.lib")  
#pragma comment( lib, "vfw32.lib" )
#pragma comment(lib,"opencv_world341.lib")
#endif // QT_DEBUG

RTSPCam::RTSPCam() 
{
	mSleepTime = chtxprGetRtspSleepTime();
}

void RTSPCam::startPlay(std::string url) {
	cap.open(url);
	if (cap.isOpened()) {
		this->start();
	}
	else {
		qDebug() << "can not open";
	}
}


void RTSPCam::run() {
	while (true)
	{
		if(cap.isOpened() && cap.read(frame))
		{
			cv::Mat *ptr = new cv::Mat(frame);
			frames.push_back(ptr);
			emit sigSendFrame(ptr);
			frames.erase(std::remove_if(frames.begin(), frames.end(), [&](const cv::Mat *p) {
				return p->empty(); }), frames.end());
		}
		this->msleep(mSleepTime);
	}
}


RTSPCam::~RTSPCam()
{
	cap.release();
}
