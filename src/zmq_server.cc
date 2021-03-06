#include "zmq_server.h"
#include <zmq.h>
#include <vector>
#include <exception>
#include <sys/time.h>



#define HEARTBEAT_LIVENESS  3               //  3-5 is reasonable
#define HEARTBEAT_INTERVAL  1000               //  sec
#define HEARTBEAT_KEY       "HEARTBEAT"
#define ZMQ_POLL_MSEC       1000            //


using namespace  std;
using namespace zmq;
static int64_t
s_clock (void)
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return  tv.tv_sec*1000;
}



void Server::Start()
{

	running_ = true;


	zmq_pollitem_t pollitems [] =
	{
		{ *back_sock_, 0, ZMQ_POLLIN, 0 },
		{ *front_sock_, 0, ZMQ_POLLIN, 0 },
	};

	int rc;
	LOG(INFO)<<"Proxy start";
	int64_t heartbeat_at = s_clock () + HEARTBEAT_INTERVAL;

	while(running_)
	{
		rc = zmq::poll(pollitems,vec_work_.size()? 2: 1,HEARTBEAT_INTERVAL);
		if (rc == -1)
		{
			LOG(ERROR)<<"Get Poll error.exit;";
			break;
		}
		if (pollitems[0].revents & ZMQ_POLLIN)
		{
			//backend
			zmq::multipart_t msg(*back_sock_);
			message_t identity = msg.pop();


			if(msg.size()==1)
			{
				message_t data = msg.remove();
				string sData((char*)data.data(),data.size());
				if(sData==HEARTBEAT_KEY)
				{
					LOG(INFO)<<"get worker heartbeat";
					MakeNewWorker(identity);
				}
			}
			else
			{
				LOG(INFO)<<"Send backend info to user via frontend";
				msg.send(*front_sock_);
			}
		}
		if (pollitems[1].revents & ZMQ_POLLIN)
		{
			zmq::multipart_t msg(*front_sock_);
			message_t worker_id;
			int ret = GetWorker(worker_id);
			if(ret != 0)
			{
				//no worker
				LOG(INFO)<<"not have woker!";
				string error="not have woker!";
				zmq::multipart_t error_msg;
				error_msg.add(msg.pop());
				error_msg.addstr("");
				error_msg.addstr(error);
				error_msg.send(*front_sock_);
			}
			else
			{
				LOG(INFO)<<"Add worker identity and send to worker via backend";
				msg.pushmem(worker_id.data(),worker_id.size());
				msg.send(*back_sock_);
			}

		}

		//send heartbeat to make sure the worker is online
		int64_t time = s_clock();
		if(time > heartbeat_at)
		{
			for (auto& itor:vec_work_)
			{
				LOG(INFO)<<"send heartbeat to one worker";
				zmq::multipart_t heartbeat_msg;
				heartbeat_msg.pushmem(itor.identity->data(),itor.identity->size());
				heartbeat_msg.addstr("HEARTBEAT");
				heartbeat_msg.send(*back_sock_);
			}
			heartbeat_at = s_clock () + HEARTBEAT_INTERVAL;

		}
		// remove expiry worker
		CheckWorker();

	}

}

void Server::MakeNewWorker(zmq::message_t& identity)
{
	bool found = false;
	for (std::vector<worker>::iterator it = vec_work_.begin(); it != vec_work_.end(); it++)
	{
		if(it->identity->equal(&identity))
		{
			LOG(INFO) << "Duplicate worker identity,Update expiry!" ;
			found = true;
			it->expiry= s_clock() + HEARTBEAT_INTERVAL * HEARTBEAT_LIVENESS;
			LOG(INFO)<<"Update Worker Expiry";
			break;
		}
	}
	if (!found)
	{
		LOG(INFO)<<"Add New Worker";
		worker new_worker;
		new_worker.identity = make_shared<message_t>(identity.data(),identity.size());
		new_worker.expiry = s_clock() + HEARTBEAT_INTERVAL * HEARTBEAT_LIVENESS;
		vec_work_.push_back(new_worker);
	}
}

int Server::GetWorker(zmq::message_t& worker_id)
{
	if(vec_work_.size()==0)
	{
		LOG(INFO)<<"GetWorker have no worker,return";
		return -1;
	}
	worker_id.copy(vec_work_[rand()%vec_work_.size()].identity.get());
	return 0;
}
void Server::CheckWorker()
{
	int64_t clock = s_clock();
	for (std::vector<worker>::iterator itor = vec_work_.begin(); itor < vec_work_.end(); itor++)
	{
		if (clock > itor->expiry)
		{
			LOG(INFO)<<"Remove one expiry worker";
			itor = vec_work_.erase(itor)-1;
		}
	}

}



