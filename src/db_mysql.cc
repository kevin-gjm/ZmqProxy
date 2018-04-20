#include "db_mysql.h"
#include "util.h"

DBMySQL:: DBMySQL() :
    iErrorNum(0), sErrorInfo("ok")
{
    mysql_library_init(0, NULL, NULL);
    mysql_init(&m_oMysqlInstance);

    // 设置字符集，否则无法处理中文
    mysql_options(&m_oMysqlInstance, MYSQL_SET_CHARSET_NAME, "utf8");
}

DBMySQL::~ DBMySQL()
{
    if(m_bIsOpen)
    {
        Close();
    }
}

// 设置连接信息
void  DBMySQL::SetMySQLConInfo(const std::string server,const std::string username, const std::string password, const std::string database, int port)
{
    m_oMysqlConInfo.server = server;
    m_oMysqlConInfo.user = username;
    m_oMysqlConInfo.password = password;
    m_oMysqlConInfo.database = database;
    m_oMysqlConInfo.port = port;
    DB_DPRINT("connect to %s:%d database:%s username:%s",server.c_str(),port,database.c_str(),username.c_str());
}

// 打开连接
bool  DBMySQL::Open()
{
    if (mysql_real_connect(&m_oMysqlInstance, m_oMysqlConInfo.server.c_str(), m_oMysqlConInfo.user.c_str(),
                           m_oMysqlConInfo.password.c_str(), m_oMysqlConInfo.database.c_str(), m_oMysqlConInfo.port, 0, 0) != NULL)
    {
        m_bIsOpen=true;
        return true;
    }
    else
    {
        ErrorIntoMySQL();
        DB_EPRINT("DBMySQL Open Error%s:%s\n",iErrorNum,sErrorInfo.c_str());
        m_bIsOpen=false;
        return false;
    }
}

// 断开连接
void  DBMySQL::Close()
{
    mysql_close(&m_oMysqlInstance);
    m_bIsOpen=false;
}

//读取数据
bool  DBMySQL::Select(const std::string& sQuerystr, std::vector<std::vector<std::string> >& vecData)
{
    try
    {
        if(!m_bIsOpen)
        {
            if(!Open())
                return false;
        }
        if (0 != mysql_query(&m_oMysqlInstance, sQuerystr.c_str()))
        {
            ErrorIntoMySQL();
            DB_EPRINT("DBMySQL Select Error%s:%s\n",iErrorNum,sErrorInfo.c_str());
            return false;
        }

        m_pResult = mysql_store_result(&m_oMysqlInstance);

        // 行列数
      //  int row = mysql_num_rows(m_pResult);

        int field = mysql_num_fields(m_pResult);

        MYSQL_ROW line = NULL;
        line = mysql_fetch_row(m_pResult);

        std::string temp;
        std::vector<std::vector<std::string> >().swap(vecData);
        while (NULL != line)
        {
            std::vector<std::string> linedata;
            for (int i = 0; i < field; i++)
            {
                if (line[i])
                {
                    temp = line[i];
                    linedata.push_back(temp);
                }
                else
                {
                    temp = "";
                    linedata.push_back(temp);
                }
            }
            line = mysql_fetch_row(m_pResult);
            vecData.push_back(linedata);
        }
        mysql_free_result(m_pResult);
        return true;
    }
    catch(std::exception &ex)
    {
        DB_EPRINT("DBMySQL Select Exception:%s\n",ex.what() );
        Close();
        m_bIsOpen= false;
        return false;
    }
}

// 其他操作
bool  DBMySQL::Query(const std::string& sQuerystr)
{
    try
    {
        if(!m_bIsOpen)
        {
            if(!Open())
                return false;
        }
        if (0 == mysql_query(&m_oMysqlInstance, sQuerystr.c_str()))
        {
            DB_DPRINT("DBMySQL Query OK");
            return true;
        }
        ErrorIntoMySQL();
        DB_EPRINT("DBMySQL Query Error%s:%s\n",iErrorNum,sErrorInfo.c_str());

        Close();
        m_bIsOpen= false;
        return false;
    }
    catch(std::exception &ex)
    {
        DB_EPRINT("DBMySQL Query Exception:%s\n",ex.what() );
        Close();
        m_bIsOpen= false;
        return false;
    }
}

// 插入并获取插入的ID,针对自动递增ID
int  DBMySQL::GetInsertAndGetID(const std::string& sQuerystr)
{
    try
    {
        if(!m_bIsOpen)
        {
            if(!Open())
                return false;
        }
        if (!Query(sQuerystr))
        {
            ErrorIntoMySQL();
            DB_EPRINT("DBMySQL GetInsertAndGetID Error%s:%s\n",iErrorNum,sErrorInfo.c_str());
            Close();
            m_bIsOpen= false;
            return ERROR_QUERY_FAIL;
        }
        // 获取ID
        return mysql_insert_id(&m_oMysqlInstance);
    }
    catch(std::exception &ex)
    {
        DB_EPRINT("DBMySQL GetInsertAndGetID Exception:%s\n",ex.what() );
        Close();
        m_bIsOpen= false;
        return ERROR_QUERY_FAIL;
    }
}

//错误信息
void  DBMySQL::ErrorIntoMySQL()
{
    iErrorNum = mysql_errno(&m_oMysqlInstance);
    sErrorInfo = std::string(mysql_error(&m_oMysqlInstance));
}
