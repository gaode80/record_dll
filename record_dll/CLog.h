#pragma once

/*******************************************************
author:       gaojy
create date:  2019-11-07
function:     ��־��
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

	int m_maxsize;              //��־�ļ�����ֽ���
	int m_islog;                //��ʶ��־��ӡ�����Ƿ����� 1--���� 0--����
	QString m_fullpath;         //���浱ǰ���ڴ�ӡ��־��ȫ·��(...//��//��//��)
	QString m_basepath;         //�����־�ļ��Ļ���·��
	QString m_logname;          //��־�ļ���
};

