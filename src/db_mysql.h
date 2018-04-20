#ifndef __MYSQL_INTERFACE_H__
#define __MYSQL_INTERFACE_H__

#include <string>
#include <vector>
#include <mysql.h>

#define ERROR_QUERY_FAIL -1 // 操作失败

#define  DB_EPRINT  LogError
#define  DB_DPRINT  LogInfo

// 定义MySQL连接信息
typedef struct
{
    std::string server;
    std::string user;
    std::string password;
    std::string database;
    int port;
}MySQLConInfo;

class DBMySQL
{
public:
    DBMySQL();
    virtual ~DBMySQL();

    DBMySQL(const DBMySQL& ) = delete;
    DBMySQL& operator = (const DBMySQL&) = delete;


    void SetMySQLConInfo(const std::string server,const std::string username, const std::string password, const std::string database, int port=3306);// 设置连接信息
    bool Open();  // 打开连接
    void Close(); // 关闭连接

    bool Select(const std::string& sQuerystr, std::vector<std::vector<std::string> >& vecData);       // 读取数据
    bool Query(const std::string& sQuerystr);     // 其他操作
    int GetInsertAndGetID(const std::string& sQuerystr);// 插入并获取插入的ID,针对自动递增ID
    void ErrorIntoMySQL();       // 错误消息

public:
    int iErrorNum;                // 错误代号
    std::string sErrorInfo;       // 错误提示

private:
    MySQLConInfo m_oMysqlConInfo;   // 连接信息
    MYSQL m_oMysqlInstance;         // MySQL对象
    MYSQL_RES *m_pResult;           // 用于存放结果
    bool m_bIsOpen{false};
};

#endif
