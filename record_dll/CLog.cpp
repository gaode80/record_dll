#include "CLog.h"

#include <direct.h>
#include <windows.h>

//CLog::CLog(int islog,const char* basepath,const char* filename,int maxsize)
CLog::CLog(int islog, QString basepath, QString filename, int maxsize)
{
	m_islog = islog;
	m_basepath = basepath;
	m_logname = filename;
	m_maxsize = maxsize;
	m_fHandle = nullptr;

	QString slogpath = MakeDir(basepath);
	if ("" != slogpath)       //路径创建成功
	{
		QString sfullname = slogpath + "\\" + filename;
		QByteArray qba = sfullname.toLatin1();
		char *chfullname = qba.data();

		struct _stat filestat = { 0 };
		if (0 == _stat(chfullname, &filestat))     //文件名存在，备份
		{
			//将旧文件备份
			struct tm ptm;
			char timestamp[32];
			memset(timestamp, 0, sizeof(timestamp));

			time_t	now = time(0);
			//ptm = localtime(&now);
			localtime_s(&ptm, &now);
			ptm.tm_year += 1900;
			ptm.tm_mon += 1;
			
			//sprintf(timestamp, "_%04d-%02d-%02d-%02d-%02d-%02d.log", ptm->tm_year, ptm->tm_mon, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
			sprintf_s(timestamp, "_%04d-%02d-%02d-%02d-%02d-%02d.log", ptm.tm_year, ptm.tm_mon, ptm.tm_mday, ptm.tm_hour, ptm.tm_min, ptm.tm_sec);
			QString bakName = sfullname + timestamp;
			QByteArray qba2 = bakName.toLatin1();
			rename(chfullname,qba2.data());
		}

		//m_fHandle = fopen(chfullname, "w");
		fopen_s(&m_fHandle, chfullname, "w");
		m_fullpath = slogpath;
	}
}

CLog::~CLog()
{
	if (m_fHandle)
	{
		fclose(m_fHandle);
	}
}

//写日志到文件里的处理函数
void CLog::Trace(const char* format, ...)
{
	if (!m_islog)
		return;

	if (!m_fHandle)
		return;

	va_list mark;
	va_start(mark, format);

	SYSTEMTIME lpsystime;
	GetLocalTime(&lpsystime);
	char chtime[32];
	memset(chtime, 0, sizeof(chtime));
	sprintf_s(chtime, "%04d-%02d-%02d %02d:%02d:%02d:%03d", lpsystime.wYear, lpsystime.wMonth, lpsystime.wDay,\
	                                                    lpsystime.wHour,lpsystime.wMinute,lpsystime.wSecond,lpsystime.wMilliseconds);
	
	fprintf(m_fHandle, "[%s] ",chtime);
	vfprintf(m_fHandle,format,mark);
	fprintf(m_fHandle, "\n");
	va_end(mark);
	fflush(m_fHandle);

	//写完日志发现已经是第二天了，就更新日志路径
	if (!IsTodayPath())
		ChangeLogPath();

	//日志文件超出设置最大值，备份文件
	struct stat buf;
	//int fdsb = fileno(m_fHandle);
	int fdsb = _fileno(m_fHandle);
	fstat(fdsb,&buf);
	if (buf.st_size >= m_maxsize)
		BackupLog();
}

//设置是否打印日志标识
void CLog::set_logflag(bool bflag)
{
	if (bflag)
		m_islog = 1;
	else
		m_islog = 0;
}

//begin private functions
//根据日志基本路径和当前日期生成日志存放路径
QString CLog::MakeDir(QString sbasepath)
{
	struct tm ptm;
	
	time_t now = time(0);
	//ptm = localtime(&now);
	localtime_s(&ptm,&now);
	int iyear = ptm.tm_year+1900;
	int imonth = ptm.tm_mon + 1;
	int iday = ptm.tm_mday;

	struct _stat filestat = { 0 };
	char* chpath = nullptr;
	QByteArray qba = sbasepath.toLatin1();
	chpath = qba.data();

	QString spath = "";

	if (_stat(chpath,&filestat) != 0)              //基本路径不存在
	{
		if (-1 == _mkdir(chpath))
			return "";

		sbasepath = sbasepath + "\\" + QString::number(iyear);
		qba = sbasepath.toLatin1();
		chpath = qba.data();
		if (-1 == _mkdir(chpath))
			return "";

		sbasepath = sbasepath + "\\" + QString::number(imonth);
		qba = sbasepath.toLatin1();
		chpath = qba.data();
		if (-1 == _mkdir(chpath))
			return "";

		sbasepath = sbasepath + "\\" + QString::number(iday);
		qba = sbasepath.toLatin1();
		chpath = qba.data();
		if (-1 == _mkdir(chpath))
			return "";
		
		spath = sbasepath;
	}//if (_stat(chpath,&filestat) != 0)
	else                                          //基本路径存在
	{
		spath = sbasepath + "\\" + QString::number(iyear);
		chpath = nullptr;
		qba = spath.toLatin1();
		chpath = qba.data();
		if (_stat(chpath, &filestat) != 0)        //年份路径不存在
		{
			if (-1 == _mkdir(chpath))
				return "";

			spath = spath + "\\" + QString::number(imonth);
			qba = spath.toLatin1();
			chpath = qba.data();
			if (-1 == _mkdir(chpath))
				return "";

			spath = spath + "\\" + QString::number(iday);
			qba = spath.toLatin1();
			chpath = qba.data();
			if (-1 == _mkdir(chpath))
				return "";
		}
		else                                    //年份路径存在
		{
			spath = spath + "\\" + QString::number(imonth);
			qba = spath.toLatin1();
			chpath = qba.data();
			if (_stat(chpath, &filestat) != 0)     //月份文件夹不存在
			{
				if (-1 == _mkdir(chpath))
					return "";

				spath = spath + "\\" + QString::number(iday);
				qba = spath.toLatin1();
				chpath = qba.data();
				if (-1 == _mkdir(chpath))
					return "";
			}//if (_stat(chpath, &filestat) != 0)
			else                                   //月份文件夹存在
			{
				spath = spath + "\\" + QString::number(iday);
				qba = spath.toLatin1();
				chpath = qba.data();
				if (_stat(chpath, &filestat) != 0)    //日文件夹不存在
				{
					if (-1 == _mkdir(chpath))
						return "";
				}
			}//end else    //月份文件夹存在
		}//end else    //年份路径存在
	}//end else   //基本路径存在

	return spath;
}//end function MakeDir

//判断当前写日志的路径是否是当天
bool CLog::IsTodayPath()
{
	struct tm ptm;
	char timestamp[32];
	memset(timestamp, 0, sizeof(timestamp));

	time_t	now = time(0);
	//ptm = localtime(&now);
	localtime_s(&ptm,&now);
	ptm.tm_year += 1900;
	ptm.tm_mon += 1;
	
	sprintf_s(timestamp, "%04d\\%d\\%d", ptm.tm_year, ptm.tm_mon, ptm.tm_mday);
	if (m_fullpath.contains(timestamp))
		return true;
	else
		return false;
}//end function IsTodayPath

//改变写日志的路径
void CLog::ChangeLogPath()
{
	if (m_fHandle != nullptr)
	{
		fclose(m_fHandle);
		m_fHandle = nullptr;
	}

	m_fullpath = MakeDir(m_basepath);
	QString snew_logfile = m_fullpath + "\\" + m_logname;
	QByteArray qba = snew_logfile.toLatin1();

	//m_fHandle = fopen(qba.data(), "w");
	fopen_s(&m_fHandle, qba.data(), "w");
}

//当日志文件大小到达设置的最大值后，备份日志文件
void CLog::BackupLog()
{
	if (m_fHandle != nullptr)
	{
		fclose(m_fHandle);
		m_fHandle = nullptr;
	}

	struct tm ptm;
	char timestamp[32];
	memset(timestamp, 0, sizeof(timestamp));

	time_t	now = time(0);
	//ptm = localtime(&now);
	localtime_s(&ptm,&now);
	ptm.tm_year += 1900;
	ptm.tm_mon += 1;

	sprintf_s(timestamp, "_%04d-%02d-%02d-%02d-%02d-%02d.log", ptm.tm_year, ptm.tm_mon, ptm.tm_mday, ptm.tm_hour, ptm.tm_min, ptm.tm_sec);
	QString bakName = m_fullpath +"\\"+ m_logname + timestamp;
	QByteArray qba = bakName.toLatin1();

	QString sold_fullname = m_fullpath + "\\" + m_logname;
	QByteArray qba2 = sold_fullname.toLatin1();
	rename(qba2.data(), qba.data());

	//m_fHandle = fopen(qba2.data(),"w");
	fopen_s(&m_fHandle, qba2.data(), "w");
}
//end private functions

