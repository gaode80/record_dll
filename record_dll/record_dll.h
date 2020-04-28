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
	//RIFF 头
	char RiffName[4];
	unsigned long nRiffLength;
	char WavName[4];// 数据类型标识符
	char FmtName[4];// 格式块中的块头
	unsigned long nFmtLength;

	//格式块中的块数据
	unsigned short nAudioFormat;
	unsigned short nChannleNumber;
	unsigned long nSampleRate;
	unsigned long nBytesPerSecond;
	unsigned short nBytesPerSample;
	unsigned short nBitsPerSample;

	//数据块中的块头
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
	void set_channelnum(int channels);       //设置通道数(如果不设置，默认为1)
	void set_samplerate(int sample_rate);    //设置采样率(支持 44100,16000,8000)
	void set_macnum(int count);              //设置麦克的数量(支持1,2 默认为1)
	void set_islog(bool bflag);              //设置是否记录日志标识
	
	void set_baidu_token(QString token);     //百度的token
	void set_xunfei_appid(QString appid);    //设置科大讯飞的appid
	void set_xunfei_secretkey(QString key);  //设置科大讯飞的秘钥

	int start_record(QString spath);         //开始录音,正常开始返回0，异常返回负数
	int end_record();                        //停止录音

	int recognize_baidu(QString spath);      //向百度提交语音识别请求
	int get_result_baidu(vector<QString> &result);

	int recognize_xunfei(QString spath);     //向科大讯飞提交语音识别请求
	int get_result_xunfei(vector<QString>& result);

	int wav2mp3(const char* inPath, const char* outPath);

	//listen
	int start_listen(int provider);                   //开始监听语音
	int stop_listen();                                //停止监听
	void get_listen_result(vector<QString> &vresult); //获取语音监听识别结果
	void write_listen_file(QString path);             //开始将监听的语音数据写入文件
	//deque<listen_info*>* get_listen_content();  //获取存放监控语音的vector指针

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

	int listen_recognize_baidu(char *pvoice,long len);      //监听状态向百度提交语音识别请求
	
private:
	int m_voice_channel_num;     //通道数
	int m_voice_sample_rate;     //采样率
	int m_mac_num;               //麦克数量
	int m_ibpos;                 //截取语音文件的起始索引
	int m_ibpos_xunfei;     
	int m_index_xunfei;          //讯飞用于语音识别临时文件的命名索引

	QAudioInput* _audioInput1;   //录音对象
	QAudioInput* _audioInput2;

	QFile m_outFile1;
	QFile m_outFile2;

	bool m_brecord_flag;          //是否正在录音标志
	bool m_baidu_recflag;         //是否在进行语音识别标志
	bool m_xunfei_recflag;        //是否在进行语音识别标志

	QString m_curr_recordpath;    //当前录音路径
	QString m_baidu_token;        //百度token
	
	QString m_recognize_temp_name;
	QString m_xunfei_appid;       //科大讯飞的appid
	QString m_xunfei_secretkey;   //科大讯飞的秘钥
	QString m_xunfei_taskid;      //科大讯飞每次识别请求的任务ID
	QFile* m_upload_file;
	SliceIdGenerator m_genid;

	Voice *m_precognize_voice;
	Voice *m_precognize_voice_xunfei;
	Voice* m_listen_voice;

	vector<QString> m_vbaidu_rec_result;   //保存百度的识别结果
	vector<QString> m_vxunfei_rec_result;  //保存讯飞的识别结果

	CLog* m_plog;
	recognize_step m_step;
	QTimer* m_timer;

	//listen
	bool m_blisten_flag;                    //监听标志
	bool m_blisten_recognize;               //监听功能是否在提交识别请求
	bool m_blisten_write_flag;              //监听语音是否写文件标志

	QAudioInput *m_listen_input;
	QIODevice* m_buffer_in;
	deque<listen_info*> md_listen_voice;    //保存监听的语音数据
	vector<QString> mv_listen_result;       //保存监听语音的识别结果

	QTimer* m_listen_timer;                 //获取监听数据定时器
	int m_iprovider;                        //识别厂商 1--百度 2--科大讯飞

	//listen-xunfei 
	QTimer* m_listen_recognize_timer;       //获取讯飞识别结果的定时器
	int m_index_xunfei_listen;              //讯飞用于语音监听临时文件的命名索引
	recognize_step m_step_listen;

	QString m_recognize_listen_name;
	QString m_xunfei_taskid_listen;
	QString m_listen_file_path;             //监听的语音信息存放路径

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
