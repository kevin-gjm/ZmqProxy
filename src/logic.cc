#include "logic.h"
#include "db_mysql.h"
#include "net_curl.h"
#include <unistd.h>
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>

using namespace rapidjson;

#define READ_INTERVAL 5
#define MEMFILE     "./device.json"

#define OPER_ADD    1
#define OPER_UPDATE 2
#define OPER_DEL    3
#define OPER_UPDATE_WITHOUT_IMG	4

#define DEVICE_INVALID  -2

#define MAX_IMG_SIZE 100*1024



Server::Server()
	:m_pMysql(new DBMySQL),
	  m_sImgIP(),
	  m_iImgPort(),
	  m_iMinUserID(numeric_limits<int>::max()),
	  m_bSetDeviceType(false),
	  m_bDBOK(false),
	  m_bImageOK(false),
	  m_bRunning(false)
{

}
Server::~Server()
{

	for(auto itor:m_vecDevice)
	{
		try
		{
			itor.oCurl.releaseCurlResource();
		}
		catch(...)
		{

		}
	}

}

void Server::Start()
{
	m_file.filename= MEMFILE;
	if(!mem_file_open(m_file))
	{
		LOG(ERROR)<<"create device.json error";
	}
	if(!(m_bDBOK&&m_bImageOK&& m_bSetDeviceType))
	{
		LOG(ERROR) << "Initialization is not complete";
		return ;
	}
	m_bRunning = true;
	int circle_time =0;
	bool need_read= true;
	while(m_bRunning)
	{
		//合并本地数据和数据库数据，并测试是否可用
		if(need_read)
		{
			LOG(INFO)<<"Check all device...";
			vector<DeviceInfo> vecFileDevice{};
			if(!ReadDeviceFromFile(vecFileDevice))
				continue;

			for(auto itor:m_vecDevice)
			{
				try
				{
					itor.oCurl.releaseCurlResource();
				}
				catch(...)
				{

				}
			}
			m_vecDevice.clear();
			if(!ReadDeviceFromDB(m_vecDevice))
				continue;

			bool bHave=false;
			for(auto& tt : vecFileDevice)
			{
				bHave= false;
				for(auto& itor :m_vecDevice)
				{
					if(tt.sDeviceUrl == itor.sDeviceUrl)
					{
						itor.iMaxUserID = tt.iMaxUserID;
						bHave = true;
						break;
					}
				}
				if(!bHave)
				{
					tt.bCanUse = false;
					m_vecDevice.push_back(tt);
				}
			}
			if(!InitDevice())
				continue;
			need_read= false;
		}

		vector<OperData> vecOperData;

		if(!GetOperDataFromDB(vecOperData))
		{
			continue;
		}
		for(auto data: vecOperData)
		{
			for(auto& device:m_vecDevice)
			{
				if(device.bCanUse&&data.area_id == device.iAreaID && data.id > device.iMaxUserID)
				{
					if(DEVICE_INVALID == SendUserChangToDevice(data,device))
					{
						device.bCanUse = false;
					}
				}
				if(device.bCanUse&& data.id > device.iMaxUserID)
					device.iMaxUserID = data.id;
			}
			m_iMinUserID = data.id;
			WriteDeviceToFile(m_vecDevice);
		}
		if(vecOperData.size()==0)
		{
			sleep(1);
			LOG(INFO)<<"get more data...";
		}



		circle_time++;
		if(circle_time > READ_INTERVAL)
		{
			need_read= true;
			circle_time=0;
		}
	}
}

void Server::Stop()
{
	m_bRunning = false;
}



bool Server::InitDB(const string& sIP,const string& sDatabaseName,const string& sUsername,const string& sPasswd,int iPort)
{
	m_pMysql->SetMySQLConInfo(sIP,sUsername,sPasswd,sDatabaseName,iPort);
	m_bDBOK = true;
	return true;
}

bool Server::InitDevice()
{
	try
	{
		for(auto& itor:m_vecDevice)
		{
			itor.oCurl.setServer(itor.sDeviceUrl,itor.iDevicePort,itor.sUsername,itor.sPasswd);
			if(0!= itor.oCurl.initCurlResource())
			{
				LOG(ERROR)<< "curl init resource error for"<<itor.sDeviceUrl;
				itor.bCanUse = false;
				continue;
			}
			itor.oCurl.setUrlPath("/action/GetSysParam");
			std::string sMsg;
			std::string sRec;
			int nCode = itor.oCurl.sendMsg(sMsg, METHOD_POST, FORMAT_JSON,sRec);

			if(nCode != 0)
			{
				LOG(ERROR)<< "Get GetSysParam Error.Return code:"<<nCode;
				if(nCode==1)
					LOG(ERROR)<<"timeout!";
				itor.bCanUse = false;
				continue;
			}


			Document document;
			document.Parse(sRec.c_str());

			Value& infoObject = document["info"];
			if (infoObject.IsObject())
			{
				int DeviceID = infoObject["DeviceID"].GetInt();
				if(DeviceID==0)
				{
					LOG(ERROR)<<"GetSysParam device id 0";
					itor.bCanUse = false;
					continue ;
				}

				itor.iDeviceID = DeviceID;
				itor.bCanUse = true;
				if(itor.iMaxUserID < m_iMinUserID)
				{
					m_iMinUserID = itor.iMaxUserID;
				}

			}
			else
			{
				LOG(ERROR)<<"GetSysParam device id error";
				itor.bCanUse = false;
				continue ;
			}
		}
		return true;
	}
	catch(exception & ex)
	{
		LOG(ERROR)<<"EXCEPTION:"<<ex.what();
		return false;
	}
}

bool Server::InitImage(const string& sIP,int iPort)
{
	m_sImgIP = sIP;
	m_iImgPort = iPort;
	m_bImageOK = true;
	return true;
}


bool Server::MakeAddPersonMsg(const int iDeviceID,const string sName,int CustomizeID,const string& sImg,string& sMsg)
{
	try
	{
		Document doc;
		rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
		Value root(kObjectType);
		root.AddMember("operator","AddPerson", allocator);


		rapidjson::Value object(rapidjson::kObjectType);
		object.AddMember("DeviceID",iDeviceID, allocator);
		object.AddMember("PersonType",0, allocator);
		if(!sName.empty())
		{
			object.AddMember("name",Value(sName.c_str(),sName.length()), allocator);
		}else
		{
			object.AddMember("name","----", allocator);
		}
		object.AddMember("gender",0, allocator);
		object.AddMember("nation",1, allocator);
		object.AddMember("cardType",0, allocator);
		object.AddMember("idCard","1111", allocator);
		object.AddMember("birthday","1990-01-01", allocator);
		object.AddMember("telnum","111", allocator);
		object.AddMember("native","111", allocator);
		object.AddMember("address","111", allocator);
		object.AddMember("notes","111", allocator);
		object.AddMember("MjCardFrom",3, allocator);
		object.AddMember("MjCardNo",0, allocator);
		object.AddMember("tempvalid",0, allocator);
		object.AddMember("CustomizeID",CustomizeID, allocator);
		object.AddMember("MjCardNo",0, allocator);
		object.AddMember("ValidBegin","1990-01-01T00:00:00", allocator);
		object.AddMember("ValidEnd","1990-01-01T00:00:00", allocator);

		root.AddMember("info", object,allocator);


		string base64img = base64_encode(sImg);
		root.AddMember("picinfo",Value(base64img.c_str(),base64img.length()),allocator);

		StringBuffer buffer;
		Writer<StringBuffer> writer(buffer);
		root.Accept(writer);
		sMsg = buffer.GetString();

		//LOG(INFO)<<"json:"<< sMsg;
		return true;
	}catch(...)
	{
		LOG(ERROR)<<"MakeAddPersonMsg Exception!";
		return false;
	}
}

bool Server::MakeEditPersonMsg(const int iDeviceID,const string sName,int CustomizeID,const string& sImg,string& sMsg)
{

	try
	{
		Document doc;
		rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
		Value root(kObjectType);
		root.AddMember("operator","EditPerson", allocator);


		rapidjson::Value object(rapidjson::kObjectType);
		object.AddMember("DeviceID",iDeviceID, allocator);
		if(!sName.empty())
		{
			object.AddMember("name",Value(sName.c_str(),sName.length()), allocator);
		}

		object.AddMember("CustomizeID",CustomizeID, allocator);
		root.AddMember("info", object,allocator);

		if(sImg.length()>0)
		{
			string base64img = base64_encode(sImg);
			root.AddMember("picinfo",Value(base64img.c_str(),base64img.length()),allocator);
		}

		StringBuffer buffer;
		Writer<StringBuffer> writer(buffer);
		root.Accept(writer);
		sMsg = buffer.GetString();

		//LOG(INFO)<<"json:"<< sMsg;
		return true;
	}
	catch(exception& ex)
	{
		LOG(ERROR)<<"EXCEPTION:"<<ex.what();
		return false;
	}
}
bool Server::MakeDeletePersonMsg(const int iDeviceID,vector<int> vecCustomizeID,string& sMsg)
{
	try
	{
		if(vecCustomizeID.size() == 0)
			return true;
		Document doc;
		rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
		Value root(kObjectType);
		root.AddMember("operator","DeletePerson", allocator);


		rapidjson::Value object(rapidjson::kObjectType);
		object.AddMember("DeviceID",iDeviceID, allocator);
		object.AddMember("TotalNum",vecCustomizeID.size(), allocator);

		Value array(kArrayType);
		for(auto itor:vecCustomizeID)
		{
			array.PushBack(itor,allocator);
		}
		object.AddMember("CustomizeID",array, allocator);


		root.AddMember("info", object,allocator);
		StringBuffer buffer;
		Writer<StringBuffer> writer(buffer);
		root.Accept(writer);
		sMsg = buffer.GetString();

		//LOG(INFO)<<"json:"<< sMsg;
		return true;

	}
	catch(exception& ex)
	{
		LOG(ERROR)<<"EXCEPTION:"<<ex.what();
		return false;
	}
}

bool Server::WriteDeviceToFile(const vector<DeviceInfo>& vecDevice)
{
	try
	{
		if(vecDevice.size() == 0)
			return true;
		Document doc;
		rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
		Value root(kObjectType);


		Value array(kArrayType);

		for(auto itor:vecDevice)
		{
			rapidjson::Value object(rapidjson::kObjectType);
			Value str_val;
			str_val.SetString(itor.sDeviceUrl.c_str(),itor.sDeviceUrl.length(),allocator);
			object.AddMember("url",str_val, allocator);
			object.AddMember("max_count",itor.iMaxUserID, allocator);

			array.PushBack(object,allocator);
		}
		root.AddMember("device",array, allocator);


		StringBuffer buffer;
		Writer<StringBuffer> writer(buffer);
		root.Accept(writer);
		string sJSON = buffer.GetString();
		LOG(INFO)<<" device json:"<< sJSON;
		mem_file_clear(m_file);
		if(!mem_file_write(m_file,0,sJSON))
		{
			LOG(ERROR)<< "Write json to file error";
			return false;
		}
		return true;

	}
	catch(exception & ex)
	{
		LOG(ERROR)<<"EXCEPTION:"<<ex.what();
		return false;
	}
}
bool Server::ReadDeviceFromFile(vector<DeviceInfo>& vecDevice)
{
	try
	{
		if(m_file.filesize==0) //empty file
			return true;
		string sJson;
		sJson.append((char*)m_file.addr,m_file.filesize);

		Document document;
		document.Parse(sJson.c_str());

		if(!document.HasMember("device"))  //empty file
		{
			LOG(ERROR)<<"json file read error";
			return false;
		}
		Value& deviceObject = document["device"];
		if (deviceObject.IsArray())
		{
			for (size_t i = 0; i < deviceObject.Size(); ++i)
			{
				DeviceInfo info;
				Value & v = deviceObject[i];
				if (v.HasMember("url") && v["url"].IsString())
				{
					info.sDeviceUrl = v["url"].GetString();
				}
				if (v.HasMember("max_count") && v["max_count"].IsInt())
				{
					info.iMaxUserID = v["max_count"].GetInt();
				}
				LOG(INFO)<<"URL:"<< info.sDeviceUrl <<" count:"<<info.iMaxUserID;
				vecDevice.push_back(info);
			}
		}
		return true;
	}
	catch(exception & ex)
	{
		LOG(ERROR)<<"EXCEPTION:"<<ex.what();
		return false;
	}

}

bool Server::ReadDeviceFromDB(vector<DeviceInfo>& vecDevice)
{
	try
	{
		string sql = "select area_id,device_ip,device_port,username,`password` from tb_monitor_device where capture = 1 and device_type =  ";
		sql.append(std::to_string(m_iDeviceType));
		vector<vector<string> > vecData;
		if(!m_pMysql->Select(sql,vecData))
			return false;

		for(auto row:vecData)
		{

			DeviceInfo info;
			info.iAreaID = std::atoi(row[0].c_str());
			info.sDeviceUrl = row[1];
			info.iDevicePort = std::atoi(row[2].c_str());
			info.sUsername = row[3];
			info.sPasswd = row[4];
			vecDevice.push_back(info);
		}

		return true;

	}
	catch(exception& ex)
	{
		LOG(ERROR)<<"EXCEPTION:"<<ex.what();
		return false;
	}
}


bool Server::GetOperDataFromDB(vector<OperData>& vecOperData)
{
	try
	{

		string sql= std::string("select img_id,user_name,img_path,area_id,oper,id from tb_user_changelog where id > ")+ std::to_string(m_iMinUserID) + " order by id limit 100 ;";
		vector<vector<string>> vecData;
		if(!m_pMysql->Select(sql,vecData))
			return false;

		for(auto itor:vecData)
		{
			OperData data;
			data.img_id = std::atoi(itor[0].c_str());
			data.user_name = itor[1];
			data.img_path = itor[2];
			data.area_id = std::atoi(itor[3].c_str());
			data.oper = std::atoi(itor[4].c_str());
			data.id = std::atoi(itor[5].c_str());
			vecOperData.push_back(data);
		}

		return true;

	}
	catch(exception& ex)
	{
		LOG(ERROR)<<"EXCEPTION:"<<ex.what();
		return false;
	}
}

int Server::SendUserChangToDevice(const OperData& data,DeviceInfo& device)
{
	try
	{

		switch(data.oper)
		{
		case OPER_ADD:
		{
			string sImg;
			int ret = GetImageFromServer(m_sImgIP,m_iImgPort,data.img_path,sImg);
			if(ret != 0)
			{
				LOG(ERROR)<<"Get image error,data id:"<<data.id<<" img path:"<< data.img_path;
				return -1;
			}
			if(sImg.length()>=MAX_IMG_SIZE)
			{
				LOG(ERROR)<<"Image size more than 100*1024,data id:"<<data.id<<" img path:"<< data.img_path;
				return -1;
			}

			string sJson ;
			if(!MakeAddPersonMsg(device.iDeviceID,data.user_name,data.img_id,sImg,sJson))
			{
				LOG(ERROR)<<"make json error:data id:"<<data.id;
				return -1;
			}
			device.oCurl.setUrlPath("/action/AddPerson");
			std::string sRec;
			int nCode = device.oCurl.sendMsg(sJson, METHOD_POST, FORMAT_JSON,sRec);
			if(nCode != 0)
			{
				return DEVICE_INVALID;
			}
			if(!GetHTTPResult(sRec))
			{
				LOG(ERROR)<<"Operate HTTP request error:data id:"<<data.id<<",customid:"<<data.img_id<<",user_name:"<<data.user_name<<",area_id:"<<data.area_id;
				return -1;
			}
			return 0;
		}
		case OPER_UPDATE:
		case OPER_UPDATE_WITHOUT_IMG:
		{

			string sImg;
			if(OPER_UPDATE_WITHOUT_IMG != data.oper)
			{
				int ret = GetImageFromServer(m_sImgIP,m_iImgPort,data.img_path,sImg);
				if(ret != 0)
				{
					LOG(ERROR)<<"Get image error,data id:"<<data.id<<" img path:"<< data.img_path;
					return -1;
				}
				if(sImg.length()>=MAX_IMG_SIZE)
				{
					LOG(ERROR)<<"Image size more than 100*1024,data id:"<<data.id<<" img path:"<< data.img_path;
					return -1;
				}
			}

			string sJson ;
			if(!MakeEditPersonMsg(device.iDeviceID,data.user_name,data.img_id,sImg,sJson))
			{
				LOG(ERROR)<<"make json error:data id:"<<data.id;
				return -1;
			}
			device.oCurl.setUrlPath("/action/EditPerson");
			std::string sRec;
			int nCode = device.oCurl.sendMsg(sJson, METHOD_POST, FORMAT_JSON,sRec);
			if(nCode != 0)
			{
				return DEVICE_INVALID;
			}
			if(!GetHTTPResult(sRec))
			{
				LOG(ERROR)<<"Operate HTTP request error:data id:"<<data.id<<",customid:"<<data.img_id<<",user_name:"<<data.user_name<<",area_id:"<<data.area_id;
				return -1;
			}
			return 0;

		}
		case OPER_DEL:
		{
			string sJson ;
			vector<int> vecCustomID;
			vecCustomID.push_back(data.img_id);
			//vecCustomID.push_back(111);
			if(!MakeDeletePersonMsg(device.iDeviceID,vecCustomID,sJson))
			{
				LOG(ERROR)<<"make json error:data id:"<<data.id;
				return -1;
			}
			device.oCurl.setUrlPath("/action/DeletePerson");
			std::string sRec;
			int nCode = device.oCurl.sendMsg(sJson, METHOD_POST, FORMAT_JSON,sRec);
			if(nCode != 0)
			{
				return DEVICE_INVALID;
			}
			if(!GetHTTPResult(sRec))
			{
				LOG(ERROR)<<"Operate HTTP request error:data id:"<<data.id<<",customid:"<<data.img_id<<",user_name:"<<data.user_name<<",area_id:"<<data.area_id;
				return -1;
			}
			return 0;
		}
		}
		return -1;

	}catch(exception& ex)
	{
		LOG(ERROR)<<"EXCEPTION:"<<ex.what();
		return -1;
	}
}

bool Server::GetHTTPResult(const string& result)
{
	try
	{
		Document document;
		document.Parse(result.c_str());
		Value& Object = document["result"];
		if(Object.IsString())
		{
			string re = Object.GetString();
			if(re == "Fail")
			{
				Value& detail = document["detail"];
				if(detail.IsString())
				{
					LOG(INFO)<<"operate error:"<< detail.GetString();
				}
				else
				{
					LOG(INFO)<<"operate error";
				}
				return false;
			}
			return true;
		}
		return false;
	}catch(exception& ex)
	{
		LOG(ERROR)<<"Exception:"<<ex.what();
		return false;
	}
}


int Server::TestDelete(const vector<int>& vecCustomID)
{
	string sJson ;

	for(auto device :m_vecDevice)
	{
		//vecCustomID.push_back(111);
		if(!MakeDeletePersonMsg(device.iDeviceID,vecCustomID,sJson))
		{

			return -1;
		}
		device.oCurl.setUrlPath("/action/DeletePerson");
		std::string sRec;
		int nCode = device.oCurl.sendMsg(sJson, METHOD_POST, FORMAT_JSON,sRec);
		if(nCode != 0)
		{
			return DEVICE_INVALID;
		}
		if(!GetHTTPResult(sRec))
		{
			return -1;
		}
	}
	return 0;

}
