#ifndef _SQLSTORE_SERVER_H_
#define _SQLSTORE_SERVER_H_
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <string>
#include <memory>
#include <vector>
#include <thread>
#include <functional>
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>


using std::string;

typedef struct worker_
{
    std::shared_ptr<zmq::message_t> identity;
    int64_t expiry;
} worker;

class Server
{
public:
    Server(zmq::context_t* context,int frontport,int backport,std::string ip=""):
		ip_(ip),
        front_port_(frontport),
        back_port_(backport),
		running_(false),
        ctx_(context),
        front_sock_(new zmq::socket_t(*ctx_,ZMQ_ROUTER)),
        back_sock_(new zmq::socket_t(*ctx_,ZMQ_ROUTER))
	{
        if(ip_.empty())
        {
            ip_="*";
        }
        std::string frontendpoint =std::string("tcp://")+ip_+":"+std::to_string(front_port_);
        std::string backendpoint =std::string("tcp://")+ip_+":"+std::to_string(back_port_);
        front_sock_->bind(frontendpoint);
        back_sock_->bind(backendpoint);
		int timeout = 3000;
        front_sock_->setsockopt(ZMQ_SNDTIMEO, &timeout, sizeof(timeout));
        back_sock_->setsockopt(ZMQ_SNDTIMEO, &timeout, sizeof(timeout));
	}
	~Server(){}

    void Start();

private:
    void MakeNewWorker(zmq::message_t& identity);
    int  GetWorker(zmq::message_t& worker_id);
    void CheckWorker();
private:
	std::string ip_;
    int front_port_;
    int back_port_;
	bool running_;
	zmq::context_t* ctx_;
    std::shared_ptr<zmq::socket_t> front_sock_;
    std::shared_ptr<zmq::socket_t> back_sock_;
    std::vector<worker> vec_work_;



};




#endif //_SQLSTORE_SERVER_H_
