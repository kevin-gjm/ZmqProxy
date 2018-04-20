#include <string>
#include <iostream>
#include<time.h>
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>
#include "zmq_server.h"
extern "C" {
#include "ini.h"

}


using namespace std;

zmq::context_t *g_pContext;


int main()
{
    google::InitGoogleLogging("GetPassRecordFatherID");
    google::InstallFailureSignalHandler();
    google::SetLogDestination(google::GLOG_INFO, "log");
    google::SetLogSymlink(google::GLOG_INFO,"");
    FLAGS_alsologtostderr = true;
    FLAGS_colorlogtostderr=true;


#if _WIN32
    ini_t *config = ini_load("D:\\SmartPark_PublicSetting.ini");
#else
    ini_t *config = ini_load("/etc/smartpark/SmartPark_PublicSetting.ini");
#endif
    if(config == NULL)
        return -1;



    int front_port,back_port;

    if (!ini_sget(config, "ProxyServer", "front_port", "%d", &front_port))return -1;
    if (!ini_sget(config, "ProxyServer", "back_port", "%d", &back_port))return -1;


    ini_free(config);

    zmq::context_t ctx;
    g_pContext = &ctx;

    srand((int)time(0));
    Server oServer(&ctx,front_port,back_port);
    oServer.Start();


    return 0;

}
