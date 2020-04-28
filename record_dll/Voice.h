#pragma once
#include <QString>

class Voice
{
public:
	Voice(QString path,int channel_num);
	Voice(char* pvoice,int ilen,int channel_num);
	~Voice();

public:
	int get_epos(int bpos);                                 //根据起始点，获取静音开始点
	void get_rec_voicedata(int bpos,int epos, char* voice); //根据起始，结束点，获取语音数据

private:
	FILE* m_fp;

	//int m_sample_rate;     //采样率
	int m_channel_num;     //通道数
	long m_voice_len;      //语音文件长度
	int m_minvoice;        //静音最大值

	char* m_chvoice;
};

