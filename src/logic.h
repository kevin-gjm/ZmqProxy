#ifndef _ASYNCUSERINFO_H_
#define _ASYNCUSERINFO_H_
#include <string>
#include <memory>
#include <vector>
#include "net_curl.h"
#include "util.h"

using namespace std;

class DBMySQL;

struct DeviceInfo
{
	NetCurl oCurl;
	string sDeviceUrl;
	int iDevicePort{0};
	string sUsername;
	string sPasswd;
	int iDeviceID{0};
	int iAreaID{0};
	int iMaxUserID{0};
	bool bCanUse{false};
};

struct OperData
{
	int img_id;
	string user_name;
	string img_path;
	int area_id;
	int oper;
	int id;
};

class Server
{
public:
	Server();
	~Server();

	void Start();
	void Stop();

	void SetDeviceType(int type)
	{
		m_iDeviceType = type;
		m_bSetDeviceType=true;
	}

	bool InitDB(const string& sIP,const string& sDatabaseName,const string& sUsername,const string& sPasswd,int iPort=3306);
	bool InitDevice();
	bool InitImage(const string& sIP,int iPort=5620);

private:
	bool MakeAddPersonMsg(const int iDeviceID,const string sName,int CustomizeID,const string& sImg,string& sMsg);
	bool MakeEditPersonMsg(const int iDeviceID,const string sName,int CustomizeID,const string& sImg,string& sMsg);
	bool MakeDeletePersonMsg(const int iDeviceID,vector<int> vecCustomizeID,string& sMsg);
	bool WriteDeviceToFile(const vector<DeviceInfo>& vecDevice);
	bool ReadDeviceFromFile(vector<DeviceInfo>& vecDevice);
	bool ReadDeviceFromDB(vector<DeviceInfo>& vecDevice);
	bool GetOperDataFromDB(vector<OperData>& vecOperData);
	int SendUserChangToDevice(const OperData& data,DeviceInfo& device);
	bool GetHTTPResult(const string& result);

	int TestDelete(const vector<int>& vecCustomID);

private:
	std::shared_ptr<DBMySQL> m_pMysql;

	vector<DeviceInfo> m_vecDevice{};


	MemFile m_file;

	string m_sImgIP;
	int m_iImgPort;

	int m_iMinUserID;
	int m_iDeviceType;

	bool m_bSetDeviceType;
	bool m_bDBOK;
	bool m_bImageOK;
	bool m_bRunning;

};


#endif //_ASYNCUSERINFO_H_
