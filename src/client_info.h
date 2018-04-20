#ifndef _SQLSTORE_CLIENT_INFO_H_
#define _SQLSTORE_CLIENT_INFO_H_
#include <memory>
#include <zmq.hpp>
#include <string>
#include <zmq_addon.hpp>

class ClientInfo
{
public:
	ClientInfo()
		:have_empty_frame_(false)
	{}
	explicit ClientInfo(zmq::message_t& ident,bool have_empty_frame):
		have_empty_frame_(have_empty_frame)
	{
		iden_.copy(&ident);
	}
	~ClientInfo()
	{}
	inline zmq::message_t GetIdentity()
	{
		zmq::message_t ret;
		ret.copy(&iden_);
		return ret;
	}
	inline bool IsHaveEmptyFrame()	{	return have_empty_frame_;	}\
	inline std::shared_ptr<zmq::multipart_t> MakeNewMsg() const
	{
		std::shared_ptr<zmq::multipart_t> ret(new zmq::multipart_t());
		if(iden_.size() != 0)
			ret->addmem(iden_.data(),iden_.size());
		if(have_empty_frame_)
			ret->addstr(std::string());
		return ret;
	}
	inline int MakeNewMsg(zmq::multipart_t& msg) const
	{
		if(iden_.size() != 0)
			msg.addmem(iden_.data(),iden_.size());
		if(have_empty_frame_)
			msg.addstr(std::string());
		return 0;
	}
private:
	zmq::message_t iden_;
	bool have_empty_frame_;
};

#endif //_SQLSTORE_CLIENT_INFO_H_
