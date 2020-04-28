#pragma once

#include "record_dll_global.h"
#include "macro.h"
#include "Voice.h"
#include "CLog.h"
#include "SliceIdGenerator.h"
//#include "listen_thread.h"

#include <QString>
#include <QtMultimedia/QAudioInput>
#include <QtNetWork/QNetworkReply>
#include <QtNetwork/QHttpMultiPart>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>

#include <deque>
using namespace std;

struct WAVHEADER
{
	//RIFF ͷ
	char RiffName[4];
	unsigned long nRiffLength;
	char WavName[4];// �������ͱ�ʶ��
	char FmtName[4];// ��ʽ���еĿ�ͷ
	unsigned long nFmtLength;

	//��ʽ���еĿ�����
	unsigned short nAudioFormat;
	unsigned short nChannleNumber;
	unsigned long nSampleRate;
	unsigned long nBytesPerSecond;
	unsigned short nBytesPerSample;
	unsigned short nBitsPerSample;

	//���ݿ��еĿ�ͷ
	char    DATANAME[4];
	unsigned long   nDataLength;
};

enum recognize_step
{
	prepare,
	upload,
	merge,
	get_result
};

typedef struct listen
{
	char* pcontent;
	int len;
}listen_info;

class _DLL_EXP record_dll:public QObject
{
	Q_OBJECT

public:
	record_dll(QString basepath = "./log");
	~record_dll();

private slots:
	void baidu_rec_finished(QNetworkReply* reply);
	
	//xunfei
	void step_finished(QNetworkReply* reply);
	void get_result();

	//listen
	void read_more();
	void check_voice();
	void listen_step_finished(QNetworkReply* reply);
	void listen_get_result();
	void listen_baidu_finished(QNetworkReply* reply);

public:
	void set_channelnum(int channels);       //����ͨ����(��������ã�Ĭ��Ϊ1)
	void set_samplerate(int sample_rate);    //���ò�����(֧�� 44100,16000,8000)
	void set_macnum(int count);              //������˵�����(֧��1,2 Ĭ��Ϊ1)
	void set_islog(bool bflag);              //�����Ƿ��¼��־��ʶ
	
	void set_baidu_token(QString token);     //�ٶȵ�token
	void set_xunfei_appid(QString appid);    //���ÿƴ�Ѷ�ɵ�appid
	void set_xunfei_secretkey(QString key);  //���ÿƴ�Ѷ�ɵ���Կ

	int start_record(QString spath);         //��ʼ¼��,������ʼ����0���쳣���ظ���
	int end_record();                        //ֹͣ¼��

	int recognize_baidu(QString spath);      //��ٶ��ύ����ʶ������
	int get_result_baidu(vector<QString> &result);

	int recognize_xunfei(QString spath);     //��ƴ�Ѷ���ύ����ʶ������
	int get_result_xunfei(vector<QString>& result);

	int wav2mp3(const char* inPath, const char* outPath);

	//listen
	int start_listen(int provider);                   //��ʼ��������
	int stop_listen();                                //ֹͣ����
	void get_listen_result(vector<QString> &vresult); //��ȡ��������ʶ����
	void write_listen_file(QString path);             //��ʼ����������������д���ļ�
	//deque<listen_info*>* get_listen_content();  //��ȡ��ż��������vectorָ��

private:
	void bak_voice_file(QString filename);
	void Mix(char sourseFile[10][SIZE_AUDIO_FRAME], int number, char* objectFile);
	void mix_voice(QString sourcefile1, QString sourcefile2, QString destfile);

	//baidu
	int recognize_baidu_private();

	//xunfei
	void prepare();
	void upload();
	void merge();
	QString hmacSha1(QByteArray key, QByteArray baseString);

	//listen
	int start_listen_record();
	void listen_prepare(char *pvoice,long len);
	void listen_upload();
	void listen_merge();

	int listen_recognize_baidu(char *pvoice,long len);      //����״̬��ٶ��ύ����ʶ������
	
private:
	int m_voice_channel_num;     //ͨ����
	int m_voice_sample_rate;     //������
	int m_mac_num;               //�������
	int m_ibpos;                 //��ȡ�����ļ�����ʼ����
	int m_ibpos_xunfei;     
	int m_index_xunfei;          //Ѷ����������ʶ����ʱ�ļ�����������

	QAudioInput* _audioInput1;   //¼������
	QAudioInput* _audioInput2;

	QFile m_outFile1;
	QFile m_outFile2;

	bool m_brecord_flag;          //�Ƿ�����¼����־
	bool m_baidu_recflag;         //�Ƿ��ڽ�������ʶ���־
	bool m_xunfei_recflag;        //�Ƿ��ڽ�������ʶ���־

	QString m_curr_recordpath;    //��ǰ¼��·��
	QString m_baidu_token;        //�ٶ�token
	
	QString m_recognize_temp_name;
	QString m_xunfei_appid;       //�ƴ�Ѷ�ɵ�appid
	QString m_xunfei_secretkey;   //�ƴ�Ѷ�ɵ���Կ
	QString m_xunfei_taskid;      //�ƴ�Ѷ��ÿ��ʶ�����������ID
	QFile* m_upload_file;
	SliceIdGenerator m_genid;

	Voice *m_precognize_voice;
	Voice *m_precognize_voice_xunfei;
	Voice* m_listen_voice;

	vector<QString> m_vbaidu_rec_result;   //����ٶȵ�ʶ����
	vector<QString> m_vxunfei_rec_result;  //����Ѷ�ɵ�ʶ����

	CLog* m_plog;
	recognize_step m_step;
	QTimer* m_timer;

	//listen
	bool m_blisten_flag;                    //������־
	bool m_blisten_recognize;               //���������Ƿ����ύʶ������
	bool m_blisten_write_flag;              //���������Ƿ�д�ļ���־

	QAudioInput *m_listen_input;
	QIODevice* m_buffer_in;
	deque<listen_info*> md_listen_voice;    //�����������������
	vector<QString> mv_listen_result;       //�������������ʶ����

	QTimer* m_listen_timer;                 //��ȡ�������ݶ�ʱ��
	int m_iprovider;                        //ʶ���� 1--�ٶ� 2--�ƴ�Ѷ��

	//listen-xunfei 
	QTimer* m_listen_recognize_timer;       //��ȡѶ��ʶ�����Ķ�ʱ��
	int m_index_xunfei_listen;              //Ѷ����������������ʱ�ļ�����������
	recognize_step m_step_listen;

	QString m_recognize_listen_name;
	QString m_xunfei_taskid_listen;
	QString m_listen_file_path;             //������������Ϣ���·��

	QFile* m_upload_listen_file;

	char* m_prevoice;
	long m_prelen;

	QNetworkAccessManager* m_prepare_Manager;
	QNetworkAccessManager* m_upload_Manager;
	QNetworkAccessManager* m_merge_Manager;
	QNetworkAccessManager* m_getresult_Manager;

	FILE* m_listen_fp;
	//FILE* m_fp; //for test
};
