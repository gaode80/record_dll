#pragma once
#include <QString>

class Voice
{
public:
	Voice(QString path,int channel_num);
	Voice(char* pvoice,int ilen,int channel_num);
	~Voice();

public:
	int get_epos(int bpos);                                 //������ʼ�㣬��ȡ������ʼ��
	void get_rec_voicedata(int bpos,int epos, char* voice); //������ʼ�������㣬��ȡ��������

private:
	FILE* m_fp;

	//int m_sample_rate;     //������
	int m_channel_num;     //ͨ����
	long m_voice_len;      //�����ļ�����
	int m_minvoice;        //�������ֵ

	char* m_chvoice;
};

