#ifdef QT_DEBUG
#include "hkcam.h"

LONG HkCam::realPlayHandle = 0;
LONG HkCam::userID = 0;
LONG HkCam::nPort = -1;
HWND  HkCam::hWnd = NULL;
cv::Mat HkCam::matImg;

HkCam::HkCam(QObject *parent)
	: QObject(parent)
{
	// 初始化  
	NET_DVR_Init();
	//设置连接时间与重连时间  
	NET_DVR_SetConnectTime(2000, 1);
	NET_DVR_SetReconnect(10000, true);

	// 注册设备  
	NET_DVR_DEVICEINFO_V30 struDeviceInfo;
	userID = NET_DVR_Login_V30(
		(char*)(chtxprGetCamIP().c_str()),
		chtxprGetCamPort(),
		(char*)(chtxprGetUserName().c_str()),
		(char*)(chtxprGetPasswd().c_str()),
		&struDeviceInfo);
	if (userID < 0)
	{
		printf("Login error, %d\n", NET_DVR_GetLastError());
		NET_DVR_Cleanup();
		return;
	}

	NET_DVR_SetExceptionCallBack_V30(0, NULL,
		exceptionCallBack, NULL);

	//启动预览并设置回调数据流   
	NET_DVR_CLIENTINFO ClientInfo;
	ClientInfo.lChannel = 1;        //Channel number 设备通道号  
	ClientInfo.hPlayWnd = NULL;     //窗口为空，设备SDK不解码只取流  
	ClientInfo.lLinkMode = 0;       //Main Stream  
	ClientInfo.sMultiCastIP = NULL;

	realPlayHandle = NET_DVR_RealPlay_V30(userID,
		&ClientInfo, realDataCallBack
		, NULL, TRUE);
	if (realPlayHandle < 0)
	{
		printf("NET_DVR_RealPlay_V30 failed! Error number: %d\n", NET_DVR_GetLastError());
		return;
	}

	this->startTimer(40);
}

void HkCam::timerEvent(QTimerEvent *event) {
	if ( matImg.empty())return;
	printf("hkcam send frame. frames=%d\n",frames.size());
	cv::Mat *pMat = new cv::Mat(matImg);
	frames.push_back(pMat);
	emit sigSendFrame(pMat);
	frames.erase(std::remove_if(frames.begin(), frames.end(), [&](const cv::Mat *p) {
		return p->empty(); }), frames.end());
}

HkCam::~HkCam()
{
	//关闭预览  
	if (!NET_DVR_StopRealPlay(realPlayHandle))
	{
		printf("NET_DVR_StopRealPlay error! Error number: %d\n", NET_DVR_GetLastError());
		return;
	}
	//注销用户  
	NET_DVR_Logout(userID);
	NET_DVR_Cleanup();
}

//解码回调 视频为YUV数据(YV12)，音频为PCM数据  
void HkCam::decFun(long nPort, char * pBuf, 
	long nSize, FRAME_INFO * pFrameInfo, 
	long nReserved1, long nReserved2)
{
	long lFrameType = pFrameInfo->nType;

	if (lFrameType == T_YV12)
	{
		cv::Mat matYCrCb;
		matYCrCb.create(cvSize(pFrameInfo->nWidth, 
			pFrameInfo->nHeight), CV_8UC3);
		int widthStep = ((matYCrCb.cols*matYCrCb.elemSize() + 3) / 4) * 4;
		yv12toYUV(matYCrCb.data, pBuf, 
			pFrameInfo->nWidth, pFrameInfo->nHeight, widthStep);

		matImg.create(cvSize(pFrameInfo->nWidth, pFrameInfo->nHeight), CV_8UC3);
		cv::cvtColor(matYCrCb, matImg, CV_YCrCb2RGB);

		//此时是YV12格式的视频数据，保存在pBuf中，可以fwrite(pBuf,nSize,1,Videofile);  
		//fwrite(pBuf,nSize,1,fp);  
	}
	/***************
	else if (lFrameType ==T_AUDIO16)
	{
	//此时是音频数据，数据保存在pBuf中，可以fwrite(pBuf,nSize,1,Audiofile);

	}
	else
	{

	}
	*******************/

}


///实时流回调  
void HkCam::realDataCallBack(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void *pUser)
{

	//PlayWt* pPlaywtHandle = static_cast<PlayWt*>(pHandle);

	DWORD dRet;
	switch (dwDataType)
	{
	case NET_DVR_SYSHEAD:    //系统头  
		if (!PlayM4_GetPort(&nPort)) //获取播放库未使用的通道号  
		{
			break;
		}
		if (dwBufSize > 0)
		{
			if (!PlayM4_OpenStream(nPort, pBuffer, dwBufSize, 1920 * 1080))
			{
				dRet = PlayM4_GetLastError(nPort);
				break;
			}
			//设置解码回调函数 只解码不显示  
			if (!PlayM4_SetDecCallBack(nPort, decFun))
			{
				dRet = PlayM4_GetLastError(nPort);
				break;
			}

			//设置解码回调函数 解码且显示  
			//if (!PlayM4_SetDecCallBackEx(nPort,DecCBFun,NULL,NULL))  
			//{  
			//  dRet=PlayM4_GetLastError(nPort);  
			//  break;  
			//}  

			//打开视频解码  
			if (!PlayM4_Play(nPort, hWnd))
			{
				dRet = PlayM4_GetLastError(nPort);
				break;
			}

			//打开音频解码, 需要码流是复合流  
			if (!PlayM4_PlaySound(nPort))
			{
				dRet = PlayM4_GetLastError(nPort);
				break;
			}
		}
		break;

	case NET_DVR_STREAMDATA:   //码流数据  
		if (dwBufSize > 0 && nPort != -1)
		{
			BOOL inData = PlayM4_InputData(nPort, pBuffer, dwBufSize);
			while (!inData)
			{
				Sleep(10);
				inData = PlayM4_InputData(nPort, pBuffer, dwBufSize);
				printf("PlayM4_InputData failed \n");
			}
		}
		break;
	}
}

void HkCam::yv12toYUV(uchar *outYuv, char *inYv12, int width, int height, int widthStep)
{
	int col, row;
	unsigned int Y, U, V;
	int tmp;
	int idx;

	for (row = 0; row < height; row++)
	{
		idx = row * widthStep;
		int rowptr = row*width;

		for (col = 0; col < width; col++)
		{
			tmp = (row / 2)*(width / 2) + (col / 2);

			Y = (unsigned int)inYv12[row*width + col];
			U = (unsigned int)inYv12[width*height + width*height / 4 + tmp];
			V = (unsigned int)inYv12[width*height + tmp];

			outYuv[idx + col * 3] = Y;
			outYuv[idx + col * 3 + 1] = U;
			outYuv[idx + col * 3 + 2] = V;
		}
	}
}

void HkCam::exceptionCallBack(DWORD dwType, LONG lUserID, LONG lHandle, void *pUser)
{
	//char tempbuf[256] = { 0 };
	switch (dwType)
	{
	case EXCEPTION_RECONNECT:    //预览时重连  
		printf("----------reconnect--------%d\n", time(NULL));
		break;
	default:
		break;
	}
}
#endif // QT_DEBUG
