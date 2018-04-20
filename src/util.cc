#include "util.h"
#include <stdio.h>
#include <iostream>
#include <string>
#include <sys/time.h>
#include <unistd.h>
#include "zmq_addon.hpp"

#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>
#include <chrono>
#include "SmartPark_FDFS_SvrInterface.h"


using namespace std;

extern  zmq::context_t *g_pContext;
extern char g_FDFS_Server_IP[64];
extern int g_FDFS_Server_Port;

void LogError(const char * pcFormat, ...)
{
	va_list args;
	va_start(args, pcFormat);
	char sBuf[1024] = {0};
	vsnprintf(sBuf, sizeof(sBuf), pcFormat, args);
	LOG(ERROR) << sBuf;
	va_end(args);
}

void LogWarning(const char * pcFormat, ...)
{
	va_list args;
	va_start(args, pcFormat);
	char sBuf[1024] = {0};
	vsnprintf(sBuf, sizeof(sBuf), pcFormat, args);
	LOG(WARNING) << sBuf;
	va_end(args);
}
void LogInfo(const char * pcFormat, ...)
{
	va_list args;
	va_start(args, pcFormat);
	char sBuf[1024] = {0};
	vsnprintf(sBuf, sizeof(sBuf), pcFormat, args);
	LOG(INFO) << sBuf;
	va_end(args);
}

long long GetTimeOfDay()
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return  tv.tv_sec;
}
long long GetZeroTimeOfDay()
{
   time_t timep;
   struct tm *p;
   time(&timep);
   printf("time() : %d \n",timep);
   p=localtime(&timep);
   p->tm_hour = 0;
   p->tm_min = 0;
   p->tm_sec  = 0;
   return  mktime(p);
}

std::string getCurrentSystemTime()
{
    auto tt = std::chrono::system_clock::to_time_t
    (std::chrono::system_clock::now());
    struct tm* ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d%02d%02d%02d%02d%02d",
        (int)ptm->tm_year + 1900,(int)ptm->tm_mon + 1,(int)ptm->tm_mday,
        (int)ptm->tm_hour,(int)ptm->tm_min,(int)ptm->tm_sec);
    return std::string(date);
}
std::string getCurrentTodayZero()
{
    auto tt = std::chrono::system_clock::to_time_t
    (std::chrono::system_clock::now());
    struct tm* ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d%02d%02d000000",
        (int)ptm->tm_year + 1900,(int)ptm->tm_mon + 1,(int)ptm->tm_mday);
    return std::string(date);
}

int ZmqFDFS_UploadFileInBuffer(const char* fileBuf,int32_t fileSize,char* fileID)
{
    LOG(INFO)<< "ZmqFDFS_UploadFileInBuffer:  in.";
    zmq::socket_t socket(*g_pContext, ZMQ_REQ);
    int timeout = 3000;
    socket.setsockopt(ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
    socket.setsockopt(ZMQ_SNDTIMEO, &timeout, sizeof(timeout));
    char ConnectParam[128];
    sprintf(ConnectParam,"tcp://%s:%d",g_FDFS_Server_IP,g_FDFS_Server_Port);
    socket.connect(ConnectParam);
    int szSReq = sizeof(SREQ_FDFS_UploadParam);
    int sendSize = szSReq + fileSize;
    zmq::message_t request(sendSize);
    SREQ_FDFS_UploadParam sReqParam;
    sReqParam.m_service = EM_FASTDFS_SERVICE::FASTDFS_UPLOAD;
    sReqParam.nFileSize = fileSize;
    strcpy(sReqParam.extName,"jpg");
    char * buf = (char*)request.data();
    memcpy(buf, &sReqParam, szSReq);
    buf += szSReq;
    memcpy(buf,fileBuf,fileSize);
    bool b = socket.send(request);
    if (b)
    {
        LOG(INFO)<<"ZmqFDFS_UploadFileInBuffer: Send success.";
    }
    else
    {
        LOG(ERROR)<<"ZmqFDFS_UploadFileInBuffer: send failed.";
        return -5;
    }
    //wait server reply.
    zmq::message_t reply;
    socket.recv(&reply);
    int recvSZ = reply.size();
    socket.close();

    if (recvSZ ==  0)
    {
        strcpy(fileID,"");
        LOG(ERROR)<<"ZmqFDFS_UploadFileInBuffer: timeout.";
        return -1;
    }
    int szRight = sizeof(SREP_FDFS_UploadResult);
    if(recvSZ != szRight)
    {
        strcpy(fileID,"");
        LOG(ERROR)<<"ZmqFDFS_UploadFileInBuffer: recv size:"<<recvSZ<<" !=right size:"<<szRight;
        return -3;
    }
    SREP_FDFS_UploadResult sResult;
    memcpy(&sResult,reply.data(),recvSZ);
    if (sResult.nResult ==EM_FDFS_Result::enum_Success_FDFS)//成功
    {
        strcpy(fileID,sResult.fileID_or_ErrorInfo);
        LOG(INFO)<< "ZmqFDFS_UploadFileInBuffer: Success. fileID:"<< fileID;
        return 0;
    }
    else
    {
        strcpy(fileID,"");
        LOG(INFO)<<"ZmqFDFS_UploadFileInBuffer: Failed err_code:"<<sResult.nResult<<" err_info:"<<sResult.fileID_or_ErrorInfo<<"-------------------------!!!!!!!!!!!!!!!!!!!!!-----------------------------------";
        return -2;
    }
}



