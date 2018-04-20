#ifndef CALM_BASE_BASE64_H_
#define CALM_BASE_BASE64_H_
#include <stdarg.h>
#include <string>


void LogError(const char * pcFormat, ...);
void LogWarning(const char * pcFormat, ...);
void LogInfo(const char * pcFormat, ...);

std::string getCurrentSystemTime();
std::string getCurrentTodayZero();

long long GetTimeOfDay();
long long GetZeroTimeOfDay();
int ZmqFDFS_UploadFileInBuffer(const char* fileBuf,int32_t fileSize,char* fileID);

#endif //CALM_BASE_BASE64_H_

