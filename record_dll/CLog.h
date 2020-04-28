#pragma once

/*******************************************************
author:       gaojy
create date:  2019-11-07
function:     日志类
********************************************************/
#include <time.h>
//#include <windows.h>
#include <QString>

#define MAX_FILE_SIZE (10 * 1024 * 1024)

class CLog
{
public:
	CLog(int islog,QString basepath,QString filename,int maxsize= MAX_FILE_SIZE);
	virtual ~CLog();

	void Trace(const char* format, ...);
	void set_logflag(bool bflag);

private:
	QString MakeDir(QString sbasepath);
	bool IsTodayPath();
	void ChangeLogPath();
	void BackupLog();

private:
	FILE* m_fHandle;

	int m_maxsize;              //日志文件最大字节数
	int m_islog;                //标识日志打印功能是否启用 1--启用 0--禁用
	QString m_fullpath;         //保存当前正在打印日志的全路径(...//年//月//日)
	QString m_basepath;         //存放日志文件的基本路径
	QString m_logname;          //日志文件名
};

