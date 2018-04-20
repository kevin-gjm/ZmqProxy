#ifndef SMART_PARK_Vehicle_MANAGEMENT_DATABASE_SERVER_INTERFACE_H
#define SMART_PARK_Vehicle_MANAGEMENT_DATABASE_SERVER_INTERFACE_H


//***************Zmq版接口定义****************************
//结果码
enum EM_RESULT
{
    RE_FAILED    = -1,		//查询失败 (数据库访问失败等问题)
    RE_DB_FAILED = -2,
    RE_EXCEPTION = -3,
    RE_DB_SUCCESSED = 0,		//查询成功（只有成功，后面才有内容，失败都是只返回错误码）
};


//车辆通行权限状态
typedef enum{
	em_Not_in_lib = 0,					//不在库中，不开闸
	em_Allow_access,					//在库中，自动开闸放行
	em_Inlib_but_time_limit			//在库中，但是时间不允许
}EM_Passerby_Status;

//车辆请求类别
typedef enum{
	//查询通行权限请求
	//S_QueryAccessRight_Req
	em_QueryAccessRight_Req = 5641,

	//查询车辆信息请求
	em_QueryVehichlInfo_Req,

	//通行记录请求
	//S_Access_Record_Req+抓拍图片
	em_Access_Record_Req,

	//时间矫正请求
	em_Time_Correct_Req,

	//心跳请求
	em_Heart_Beat_Req
}EM_Vehicle_REQ_TYPE;

//车辆应答类别
typedef enum{
	//未知请求，或者请求数据校验失败
	em_Unknown_Reply = 0,

	//查询通行权限应答
	//S_QueryAccessRight_Reply
	em_QueryAccessRight_Reply,
	
	//查询车辆信息应答
	//S_QueryVehicleInfo_Reply+车辆信息结构体*n
	em_QueryVehichlInfo_Reply,

	//通行记录应答
	//S_Access_Record_Reply
	em_Access_Record_Reply,

	//时间矫正应答
	//S_Time_Correct_Reply
	em_Time_Correct_Reply,

	//心跳请求
	em_Heart_Beat_Reply
}EM_Vehicle_REP_TYPE;


//字节对齐统一:begin
#pragma pack(push)
#pragma pack(1)

typedef struct{
	int year;
	int month;
	int day;
	int hour;
	int minute;
	int second;
}S_DateTime;

typedef struct{
    char sPlate_No[20];		//车牌号码
	int nFlag_Inner;		//车辆所属
	int nFlag_Military;
	S_DateTime sRegister_DateTime;

	//内部军牌信息
	S_DateTime sMilitary_OutDate;
	int nMilitary_OutHours;

	//内部民牌信息
	S_DateTime sCivil_Expired_Date;

	//访客车辆
	S_DateTime sVisit_Date;
	int nVisit_Hours;
	int nVisit_Enter_Times;
}S_VehicleInfo;

//通行权限请求结构体
typedef struct{
	EM_Vehicle_REQ_TYPE emReq;	//请求类型=em_QueryAccessRight_Req
    char sPlate_No[20];			//车牌号
	bool bDerection;		//通行方向 0入 1出
}S_QueryAccessRight_Req;

//通行权限应答结构体
typedef struct{
	EM_Vehicle_REP_TYPE emReply;	//应答类型=em_QueryAccessRight_Reply
	EM_Passerby_Status em_PasserbyStatus;		//通行权限
}S_QueryAccessRight_Reply;

//查询车辆信息应答结构体
typedef struct{
	EM_Vehicle_REP_TYPE emReply;	//应答类型=em_QueryVehichlInfo_Reply
	int nVehicleNumber;
}S_QueryVehicleInfo_Reply;

//通行记录请求结构体
typedef struct{
	EM_Vehicle_REQ_TYPE emReq;	//请求类型=em_Access_Record_Req
    char sPlate_No[20];			//车牌号
	bool bDerection;		//通行方向 0入 1出
	S_DateTime dataTime;	//通行时间
	int nFlag_military;				//车牌类型	0军牌 1民牌
	int nPass_Status;				//通行状态 0自动放行， 1门卫开闸放行
	int nPicSize;						//图片大小
}S_AccessRecord_Req;

//通行记录应答结构体
typedef struct{
	EM_Vehicle_REP_TYPE emReply;	//应答类型=em_Access_Record_Reply
	int nStatus;		//执行状态
}S_AccessRecord_Reply;

//时间矫正应答包
typedef struct{
	EM_Vehicle_REP_TYPE emRepType;		//应答类型=em_Time_Correction_Reply
	S_DateTime sDateTime;
}S_TimeCorrection_Reply;

//字节对齐统一:end
#pragma pack(pop)


#endif
