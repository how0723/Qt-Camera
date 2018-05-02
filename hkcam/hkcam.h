#ifdef QT_DEBUG
#pragma once

#include "common/common.h"

class HkCam : public QObject
{
	Q_OBJECT

public:
	HkCam(QObject *parent=nullptr);
	~HkCam();
signals:
	void sigSendFrame(cv::Mat* mat);
protected:
	void timerEvent(QTimerEvent *event);
private:
	static void yv12toYUV(uchar *outYuv, char *inYv12, int width, int height, int widthStep);

	//实时流回调  
	static void realDataCallBack(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void *pUser);
	//解码回调 视频为YUV数据(YV12)，音频为PCM数据  
	static void decFun(long nPort, char * pBuf, long nSize, FRAME_INFO * pFrameInfo, long nReserved1, long nReserved2);
	static void exceptionCallBack(DWORD dwType, LONG lUserID, LONG lHandle, void *pUser);

	static LONG realPlayHandle;
	static LONG userID;

	static int iPicNum;//Set channel NO.  
	static LONG nPort;
	static HWND hWnd;
	static cv::Mat matImg;

	std::vector<cv::Mat*> frames;
	//QImage cvMat2QImage(const cv::Mat& mat);
};
#endif // QT_DEBUG
