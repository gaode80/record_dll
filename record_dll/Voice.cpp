#include "Voice.h"

#include <cstdlib>

Voice::Voice(QString path,int channel_num)
{
	assert(path != "");

	m_channel_num = channel_num;
	m_fp = fopen(path.toLocal8Bit(), "rb");
	fseek(m_fp, 0L, SEEK_END);
	m_voice_len = ftell(m_fp);
	fseek(m_fp, 0L, SEEK_SET);
	m_chvoice = new char[m_voice_len+1];
	memset(m_chvoice, 0, m_voice_len + 1);
	int ilen = fread(m_chvoice, 1, m_voice_len, m_fp);
	fclose(m_fp);

	m_minvoice = 20;

	/*int istart = 0;
	int inum = 0;
	for (int i = 0; i < m_voice_len; i = i + 2)
	{
		short s1 = m_chvoice[i];
		s1 = s1 & 0x00ff;
		short s2 = m_chvoice[i + 1];
		s2 = s2 & 0x00ff;

		s2 = s2 << 8;
		short s3 = s1 | s2;

		if (abs(s3) < 1000)
		{
			++inum;
		}
		if (inum > 10)
		{
			for(int j=istart;)
		}

	}
	FILE* fp1 = fopen("d:/1234.wav", "wb");
	fwrite(m_chvoice, 1, m_voice_len, fp1);
	fclose(fp1);
	*/
}

Voice::Voice(char* pvoice, int ilen, int channel_num)
{
	assert(pvoice);
	m_fp = nullptr;

	m_channel_num = channel_num;
	m_voice_len = ilen;

	m_chvoice = new char[ilen+1];
	memcpy(m_chvoice,pvoice,ilen);

	m_minvoice = 20;
}

Voice::~Voice()
{
	if (m_fp)
	{
		fclose(m_fp);
		m_fp = nullptr;
	}

	if (m_chvoice)
	{
		delete[] m_chvoice;
		m_chvoice = nullptr;
	}
}

int Voice::get_epos(int bpos)
{
	if (m_voice_len == bpos)
		return -1;

	bool bvoice_begin = false;     //非静音点是否开始
	bool bsilence_begin = false;   //静音点开始
	int ipoint = 0;
	int ivoicenum = 0;		       //声音点累加
	int isilence_voicenum = 0;     //静音累加点
	int epos = 0;

	int i = 0;
	int increase_num = 0;
	if (1 == m_channel_num)
		increase_num = 2;
	else if (2 == m_channel_num)
		increase_num = 4;

	for (i = bpos; i < m_voice_len; i = i + increase_num)
	{
		short s1 = m_chvoice[i];
		s1 = s1 & 0x00ff;
		short s2 = m_chvoice[i + 1];
		s2 = s2 & 0x00ff;

		s2 = s2<<8;
		short s3 = s1 | s2;

		if(abs(s3)> m_minvoice && !bvoice_begin)
		{
			ivoicenum++;            //非静音点
			if(ivoicenum>=100)
				bvoice_begin = true;
		}
		else if ((abs(s3)) < m_minvoice && !bvoice_begin)
		{
			ivoicenum = 0;
		}
		else if ((abs(s3)) < m_minvoice && bvoice_begin)
		{
			isilence_voicenum++;
			bsilence_begin = true;
			if (isilence_voicenum >= 100)
			{
				epos = i;
				break;
			}	
		}
		else if ((abs(s3)) > m_minvoice && bsilence_begin)
		{
			isilence_voicenum = 0;
			bsilence_begin = false;
		}
	}//end for...

	if (i >= m_voice_len-4 && bvoice_begin)
		epos = m_voice_len - 1;

	return epos;
}

void Voice::get_rec_voicedata(int bpos, int epos,char* voice)
{
	if (nullptr == voice)
		return;

	if (bpos >= epos)
		return;

	size_t slen = epos - bpos;
	memcpy(voice, m_chvoice+bpos, slen);
	
	return;
}
