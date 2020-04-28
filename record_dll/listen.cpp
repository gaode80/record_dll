#include "record_dll.h"
#include "MD5.h"
#include <windows.h>

#include <QTextCodec>

//begin public functions
//开始监听语音
//content--需要监听的语音内容
int record_dll::start_listen(int provider)
{
	if (1 != provider && 2 != provider)
		m_iprovider = 2;
	else
		m_iprovider = provider;

	m_blisten_recognize = false;
	m_blisten_write_flag = false;
	m_listen_file_path = "";
	m_listen_fp = nullptr;

	int iret = start_listen_record();
	if (0 == iret)
	{
		m_listen_timer = new QTimer();
		//设置定时器是否为单次触发。默认为 false 多次触发   
		//m_listen_timer->setSingleShot(true);

		//启动或重启定时器, 并设置定时器时间：毫秒   
		m_listen_timer->start(2000);

		//定时器触发信号槽   
		connect(m_listen_timer, SIGNAL(timeout()), this, SLOT(check_voice()));
	}

	mv_listen_result.clear();
	return iret;
}

//停止监听
int record_dll::stop_listen()
{
	if (!m_blisten_flag)
		return 0;

	m_listen_timer->stop();
	delete m_listen_timer;
	m_listen_timer = nullptr;

	m_listen_input->stop();
	m_buffer_in->close();
	delete m_listen_input;
	m_listen_input = nullptr;

	m_blisten_flag = false;

	listen_info* pinfo = nullptr;
	for (int i=0; i<md_listen_voice.size(); ++i)
	{
		pinfo = md_listen_voice.at(i);
		if (pinfo->pcontent)
		{
			delete []pinfo->pcontent;
			pinfo->pcontent = nullptr;
		}
		delete pinfo;
		pinfo = nullptr;
	}
	md_listen_voice.clear();

	if (m_listen_fp)
	{
		fclose(m_listen_fp);
		m_listen_fp = nullptr;
	}

	//for test
	//if (m_fp)
	//{
	//	fclose(m_fp);
	//	m_fp = nullptr;
	//}
	return 0;
}

//获取识别结果，被获取过的结果就不会再保存
void record_dll::get_listen_result(vector<QString>& vresult)
{
	for (auto it : mv_listen_result)
		vresult.push_back(it);

	mv_listen_result.clear();
}

void record_dll::write_listen_file(QString path)
{
	m_listen_file_path = path;
	m_listen_fp = fopen(m_listen_file_path.toLocal8Bit(), "wb");
	assert(m_listen_fp);

	m_blisten_write_flag = true;
}
//end public functions


//begin slot functions
void record_dll::read_more()
{
	long len = m_listen_input->bytesReady();
	listen_info* plisten_info = nullptr;
	plisten_info = new listen_info;
	assert(plisten_info);

	plisten_info->pcontent = nullptr;
	plisten_info->pcontent = new char[len + 1];
	assert(plisten_info->pcontent);

	memset(plisten_info->pcontent, 0, len + 1);
	long l = m_buffer_in->read(plisten_info->pcontent, len);
	plisten_info->len = len;
	md_listen_voice.push_back(plisten_info);

	if (m_blisten_write_flag)
		fwrite(plisten_info->pcontent, 1, len, m_listen_fp);
	
	/*static int index = 1;
	if (index == 1)
	{
		QString sname = "d:\\123.wav";
		m_fp = fopen(sname.toLocal8Bit(), "wb");
		fwrite(plisten_info->pcontent, 1, len, m_fp);
	
		++index;
	}
	else
		fwrite(plisten_info->pcontent, 1, len, m_fp);
	*/	
}

void record_dll::check_voice()
{
	if (m_blisten_recognize)
		return;

	listen_info* pinfo = nullptr;
	long len = 0;
	for (int i = 0; i < md_listen_voice.size(); ++i)
	{
		pinfo = md_listen_voice.at(i);
		len += pinfo->len;
	}

	pinfo = nullptr;
	long index = 0; 
	char* pcontent = new char[len+1+m_prelen];
	memset(pcontent, 0, len+1+m_prelen);

	if (m_prevoice)
	{
		memcpy(&pcontent[index],m_prevoice,m_prelen); 
		delete []m_prevoice;
		m_prevoice = nullptr;

		index = m_prelen;
	}

	int voice_count = md_listen_voice.size();
	for (int j = 0; j < voice_count; ++j)
	{
		pinfo = md_listen_voice.at(j);
		memcpy(&pcontent[index], pinfo->pcontent, pinfo->len);
		index += pinfo->len;
	}

	Voice listen_voice(pcontent,index, m_voice_channel_num);
	long iepos = listen_voice.get_epos(0);
	if (0 == iepos)   //全是静音
	{
		pinfo = nullptr;
		for (int k = 0; k < voice_count; ++k)
		{
			pinfo = md_listen_voice.front();
			md_listen_voice.pop_front();

			delete []pinfo->pcontent;
			delete pinfo;
			pinfo = nullptr;
		}
		md_listen_voice.clear();
	}
	else if (index-1 == iepos)   //说话还没有结束
	{
		//m_plog->Trace("1234567");
	}
	else                     //已经开始说话，并且结束
	{
		
		//保存说话结束后的语音，用作下次识别(因为可能有下一次说话的语音在里面)
		if (index - iepos > 100)
		{
			if (0 != iepos % 2)
				iepos += 1;
			m_prevoice = new char[index - iepos];
			memset(m_prevoice, 0, index - iepos);
			m_prelen = index - iepos-1;
			if (0 != m_prelen % 2)
				m_prelen -= 1;
			memcpy(m_prevoice, &pcontent[iepos], m_prelen);  
		}

		char* pvoice = new char[iepos + 1];
		listen_voice.get_rec_voicedata(0,iepos,pvoice);
		
		if(1 == m_iprovider)
			listen_recognize_baidu(pvoice,iepos);
		else if(2==m_iprovider)
			listen_prepare(pvoice,iepos);

		delete pvoice;
		pvoice = nullptr;

		pinfo = nullptr;
		for (int k = 0; k < voice_count; ++k)
		{
			pinfo = md_listen_voice.front();
			md_listen_voice.pop_front();

			delete []pinfo->pcontent;
			delete pinfo;
			pinfo = nullptr;
		}
		md_listen_voice.clear();
	}	

	delete []pcontent;
	pcontent = nullptr;
}

void record_dll::listen_step_finished(QNetworkReply* reply)
{
	// 获取http状态码
	QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
	if (!statusCode.isValid())
	{
		m_plog->Trace("xunfei listen recognize fail,statusCode is invalid!!");
		m_blisten_recognize = false;
		return;
	}
	if (statusCode.toUInt() != 200)
	{
		m_plog->Trace("xunfei listen recognize fail,return code is %d", statusCode.toUInt());
		m_blisten_recognize = false;
		return;
	}

	QNetworkReply::NetworkError err = reply->error();
	if (err != QNetworkReply::NoError)
	{
		m_plog->Trace("xunfei listen recognize fail,err is %d", err);
		m_blisten_recognize = false;
		return;
	}
	else
	{
		// 获取返回内容
		QByteArray bytes = reply->readAll();
		QString result(bytes);

		QJsonDocument jsonDocument = QJsonDocument::fromJson(result.toUtf8());
		QJsonObject qobj = jsonDocument.object();
		QJsonValue qvalue = qobj.value("ok");

		switch (m_step_listen)
		{
		case recognize_step::prepare:
		{
			if (0 == qvalue.toInt())
			{
				QJsonValue qvalue = qobj.value("data");
				m_xunfei_taskid_listen = qvalue.toString();

				listen_upload();
			}
			else
			{
				QJsonValue qerr_no = qobj.value("err_no");
				QJsonValue qfailed = qobj.value("failed");

				QString info = "err_no=";
				info += QString::number(qerr_no.toInt());
				info += " failed=";
				info += qfailed.toString();
				m_plog->Trace("xunfei listen recognize fail in prepare %s", info.toLocal8Bit().data());
				m_blisten_recognize = false;
			}
		}
			break;
		case recognize_step::upload:
		{
			if (m_upload_listen_file)
			{
				m_upload_listen_file->close();
				delete m_upload_listen_file;
				m_upload_listen_file = nullptr;
			}
			if (0 == qvalue.toInt())
				listen_merge();
			else
			{
				QJsonValue qerr_no = qobj.value("err_no");
				QJsonValue qfailed = qobj.value("failed");

				QString info = "err_no=";
				info += QString::number(qerr_no.toInt());
				info += " failed=";
				info += qfailed.toString();
				m_plog->Trace("xunfei listen recognize fail in upload %s", info.toLocal8Bit().data());
				m_blisten_recognize = false;
			}
		}
			break;
		case recognize_step::merge:
		{
			if (0 == qvalue.toInt())
			{
				m_listen_recognize_timer = new QTimer;
				//设置定时器是否为单次触发。默认为 false 多次触发   
				m_listen_recognize_timer->setSingleShot(true);

				//启动或重启定时器, 并设置定时器时间：毫秒   
				m_listen_recognize_timer->start(2000);

				//定时器触发信号槽   
				connect(m_listen_recognize_timer, SIGNAL(timeout()), this, SLOT(listen_get_result()));
			}
			else
			{
				QJsonValue qerr_no = qobj.value("err_no");
				QJsonValue qfailed = qobj.value("failed");

				QString info = "err_no=";
				info += QString::number(qerr_no.toInt());
				info += " failed=";
				info += qfailed.toString();
				m_plog->Trace("xunfei listen recognize fail in merge %s", info.toLocal8Bit().data());
				m_blisten_recognize = false;
			}
		}
			break;
		case recognize_step::get_result:
		{
			if (m_listen_recognize_timer)
			{
				delete m_listen_recognize_timer;
				m_listen_recognize_timer = nullptr;
			}

			QString qcontent = "";
			if (0 == qvalue.toInt())
			{
				QJsonValue qjdata = qobj.value("data");
				QJsonDocument doc_data = QJsonDocument::fromJson(qjdata.toString().toUtf8());
				QJsonObject data_obj = doc_data.object();

				QJsonValue qj_result = data_obj.value("audio_result");
				QJsonDocument document2 = QJsonDocument::fromJson(qj_result.toString().toUtf8());
				QJsonArray json_array = document2.array();

				QString onebest = "";
				for (int i = 0; i < json_array.size(); ++i)
				{
					QJsonObject obj_temp = json_array.at(i).toObject();
					onebest = obj_temp.value("onebest").toString();

					onebest.remove(QString::fromLocal8Bit("？"), Qt::CaseInsensitive);
					onebest.remove(QString::fromLocal8Bit("！"), Qt::CaseInsensitive);
					onebest.remove(QString::fromLocal8Bit("，"), Qt::CaseInsensitive);
					onebest.remove(QString::fromLocal8Bit("。"), Qt::CaseInsensitive);
					onebest.remove(QString::fromLocal8Bit("、"), Qt::CaseInsensitive);

					onebest.remove(QString::fromLocal8Bit("?"), Qt::CaseInsensitive);
					onebest.remove(QString::fromLocal8Bit("!"), Qt::CaseInsensitive);
					onebest.remove(QString::fromLocal8Bit(","), Qt::CaseInsensitive);
					onebest.remove(QString::fromLocal8Bit("."), Qt::CaseInsensitive);

					qcontent += onebest;
				}
				mv_listen_result.push_back(qcontent);
				int ret = remove(m_recognize_listen_name.toLocal8Bit());

				m_blisten_recognize = false;

			}//end if (0 == qvalue.toInt())
			else
			{
				QJsonValue qerr_no = qobj.value("err_no");
				if (26605 == qerr_no.toInt())    //26605--任务处理中
				{
					m_listen_recognize_timer = new QTimer;
					//设置定时器是否为单次触发。默认为 false 多次触发   
					m_listen_recognize_timer->setSingleShot(true);

					//启动或重启定时器, 并设置定时器时间：毫秒   
					m_listen_recognize_timer->start(1000);

					//定时器触发信号槽   
					connect(m_listen_recognize_timer, SIGNAL(timeout()), this, SLOT(listen_get_result()));
				}
				else
				{
					QJsonValue qerr_no = qobj.value("err_no");
					QJsonValue qfailed = qobj.value("failed");

					QString info = "err_no=";
					info += QString::number(qerr_no.toInt());
					info += " failed=";
					info += qfailed.toString();
					m_plog->Trace("xunfei get listen result fail %s", info.toLocal8Bit().data());
				
					m_blisten_recognize = false;
				}
			}
		}
			break;
		default:
			break;
		}
	}
}

void record_dll::listen_get_result()
{
	QNetworkRequest request;
	QNetworkAccessManager* naManager = new QNetworkAccessManager(this);
	QMetaObject::Connection connRet = QObject::connect(naManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(listen_step_finished(QNetworkReply*)));
	//QMetaObject::Connection connRet = QObject::connect(m_getresult_Manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(listen_step_finished(QNetworkReply*)));
	Q_ASSERT(connRet);

	request.setUrl(QUrl("http://raasr.xfyun.cn/api/getResult"));
	request.setRawHeader("Content-Type", "application/x-www-form-urlencoded; charset=UTF-8");

	QString sts = "", ssign = "";
	while (true)
	{
		time_t now_time = time(NULL);
		sts = QString::number(now_time);

		QString qsbefore_sign = m_xunfei_appid + sts;
		string sbefore_sign = qsbefore_sign.toLocal8Bit();
		MD5 md5;
		md5.update(sbefore_sign);
		sbefore_sign = md5.toString();
		ssign = hmacSha1(m_xunfei_secretkey.toUtf8(), QString::fromStdString(sbefore_sign).toUtf8());

		int ipos = ssign.indexOf("+");
		if (-1 == ipos)
			break;
		else
			Sleep(100);
	}

	QByteArray post_data;
	post_data.append("signa=");
	post_data.append(ssign);
	post_data.append("&ts=");
	post_data.append(sts);
	post_data.append("&app_id=");
	post_data.append(m_xunfei_appid);
	post_data.append("&task_id=");
	post_data.append(m_xunfei_taskid_listen);

	QByteArray len;
	len.append(QString::number(post_data.length()));
	request.setRawHeader("Content-Length", len);
	request.setRawHeader("Chunked", "false");

	QNetworkReply* reply = naManager->put(request, post_data);
	//QNetworkReply* reply = m_getresult_Manager->put(request, post_data);
	m_step_listen = recognize_step::get_result;
}

void record_dll::listen_baidu_finished(QNetworkReply* reply)
{
	QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
	if (!statusCode.isValid())
	{
		m_plog->Trace("baidu listen recognize fail,statusCode is invalid!!");
		return;
	}

	if (statusCode.toUInt() != 200)
	{
		m_plog->Trace("baidu listen recognize fail,return code is %d", statusCode.toUInt());
		return;
	}

	QNetworkReply::NetworkError err = reply->error();
	if (err != QNetworkReply::NoError)
	{
		m_plog->Trace("baidu listen recognize fail,err is %d", err);
		return;
	}
	else
	{
		// 获取返回内容
		QByteArray bytes = reply->readAll();
		QString result(bytes);

		QTextCodec* tc = QTextCodec::codecForName("UTF-8");
		QJsonDocument jsonDocument = QJsonDocument::fromJson(result.toUtf8());
		QJsonObject qobj = jsonDocument.object();
		QJsonValue err_no = qobj.value("err_no");

		if (0 == err_no.toInt())
		{
			QJsonValue qvalue = qobj.value("result");

			QString qs_result = "";
			QJsonArray res_array = qvalue.toArray();
			int icount = res_array.count();
			for (int i = 0; i < icount; ++i)
				qs_result += res_array.at(i).toString();

			qs_result.remove(QString::fromLocal8Bit("？"), Qt::CaseInsensitive);
			qs_result.remove(QString::fromLocal8Bit("！"), Qt::CaseInsensitive);
			qs_result.remove(QString::fromLocal8Bit("，"), Qt::CaseInsensitive);
			qs_result.remove(QString::fromLocal8Bit("。"), Qt::CaseInsensitive);
			qs_result.remove(QString::fromLocal8Bit("、"), Qt::CaseInsensitive);

			qs_result.remove(QString::fromLocal8Bit("?"), Qt::CaseInsensitive);
			qs_result.remove(QString::fromLocal8Bit("!"), Qt::CaseInsensitive);
			qs_result.remove(QString::fromLocal8Bit(","), Qt::CaseInsensitive);
			qs_result.remove(QString::fromLocal8Bit("."), Qt::CaseInsensitive);

			mv_listen_result.push_back(qs_result);
			m_plog->Trace("baidu listen recognize result is %s", qs_result.toLocal8Bit().data());
		}
		else
		{
			QJsonValue qfailed = qobj.value("err_msg");

			QString info = "err_no=";
			info += QString::number(err_no.toInt());
			info += " msg=";
			info += qfailed.toString();
			m_plog->Trace("baidu recognize fail %s", info.toLocal8Bit().data());
		}
		m_blisten_recognize = false;
	}//end else
}
//end slot functions


//begin private functions
int record_dll::start_listen_record()
{
	if (m_brecord_flag)
	{
		m_plog->Trace("curr status is recording, can`t listen!!");
		return -3;
	}
	if (m_blisten_flag)
	{
		m_plog->Trace("curr status is listening,please stop listen before!!");
		return -1;
	}	

	QAudioFormat audioFormat;
	audioFormat.setByteOrder(QAudioFormat::LittleEndian);
	audioFormat.setCodec("audio/pcm");
	audioFormat.setSampleSize(16);
	audioFormat.setChannelCount(m_voice_channel_num);
	audioFormat.setSampleRate(m_voice_sample_rate);
	//audioFormat.setSampleSize(m_voice_bit_len);
	audioFormat.setSampleType(QAudioFormat::SignedInt);

	QAudioDeviceInfo devInfo;
	//判断设备，查看是否存在
	devInfo = QAudioDeviceInfo::defaultInputDevice();
	if (devInfo.isNull())
	{
		m_plog->Trace("listen no find record device!!");
		return -2;
	}

	//不支持格式，使用最接近格式
	if (!devInfo.isFormatSupported(audioFormat))          //当前使用设备是否支持
		audioFormat = devInfo.nearestFormat(audioFormat); //转换为最接近格式

	//内存的IO对象
	m_listen_input = new QAudioInput(devInfo, audioFormat, this);
	assert(m_listen_input);
	
	m_listen_input->setBufferSize(5120);
	m_buffer_in = m_listen_input->start();
	connect(m_buffer_in, SIGNAL(readyRead()), SLOT(read_more()));
	m_blisten_flag = true;

	return 0;
}

void record_dll::listen_prepare(char* pvoice, long len)
{
	m_blisten_recognize = true;

	//添加wav文件头
	WAVHEADER wavHeader;
	qstrcpy(wavHeader.RiffName, "RIFF");
	qstrcpy(wavHeader.WavName, "WAVE");
	qstrcpy(wavHeader.FmtName, "fmt ");
	qstrcpy(wavHeader.DATANAME, "data");

	int nAudioFormat = 1;
	wavHeader.nFmtLength = 16;
	wavHeader.nAudioFormat = nAudioFormat;
	wavHeader.nBitsPerSample = 16;
	wavHeader.nChannleNumber = m_voice_channel_num;
	wavHeader.nSampleRate = m_voice_sample_rate;

	wavHeader.nBytesPerSample = wavHeader.nChannleNumber * wavHeader.nBitsPerSample / 8;
	wavHeader.nBytesPerSecond = wavHeader.nSampleRate * wavHeader.nChannleNumber * wavHeader.nBitsPerSample / 8;
	wavHeader.nRiffLength = len - 8 + sizeof(WAVHEADER);
	wavHeader.nDataLength = len;

	m_recognize_listen_name = "./xunfei_listen_" + QString::number(m_index_xunfei_listen) + ".wav";
	FILE* fp1 = fopen(m_recognize_listen_name.toLocal8Bit(), "wb");
	fwrite(&wavHeader, 1, sizeof(WAVHEADER), fp1);
	fwrite(pvoice, 1, len, fp1);
	fseek(fp1, 0, SEEK_END);
	int size = ftell(fp1);
	fseek(fp1, 0, SEEK_SET);
	fclose(fp1);

	++m_index_xunfei_listen;
	if (10000 == m_index_xunfei_listen)
		m_index_xunfei = 1;

	QNetworkRequest request;
	QNetworkAccessManager* naManager = new QNetworkAccessManager(this);
	QMetaObject::Connection connRet = QObject::connect(naManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(listen_step_finished(QNetworkReply*)));
	//QMetaObject::Connection connRet = QObject::connect(m_prepare_Manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(listen_step_finished(QNetworkReply*)));
	Q_ASSERT(connRet);

	request.setUrl(QUrl("http://raasr.xfyun.cn/api/prepare"));
	request.setRawHeader("Content-Type", "application/x-www-form-urlencoded; charset=UTF-8");

	QString sts = "", ssign = "";
	while (true)
	{
		time_t now_time = time(NULL);
		sts = QString::number(now_time);

		QString qsbefore_sign = m_xunfei_appid + sts;
		string sbefore_sign = qsbefore_sign.toLocal8Bit();
		MD5 md5;
		md5.update(sbefore_sign);
		sbefore_sign = md5.toString();
		ssign = hmacSha1(m_xunfei_secretkey.toUtf8(), QString::fromStdString(sbefore_sign).toUtf8());
		int ipos = ssign.indexOf("+");
		if (-1 == ipos)
			break;
		else
			Sleep(100);
	}

	QByteArray post_data;
	post_data.append("file_len=");
	post_data.append(QString::number(size));
	post_data.append("&signa=");
	post_data.append(ssign);
	post_data.append("&ts=");
	post_data.append(sts);
	post_data.append("&app_id=");
	post_data.append(m_xunfei_appid);
	post_data.append("&slice_num=");
	post_data.append(QString::number(1));//inum
	post_data.append("&file_name=");
	post_data.append(m_recognize_listen_name);
	//post_data.append(".wav");
	//post_data.append("&has_seperate=true"); 
	//post_data.append("&has_participle=true");
	//post_data.append("&max_alternatives=3");

	QString skey_words = "";
	skey_words = QString::fromLocal8Bit("开始录音");
	post_data.append("&has_sensitive=true&sensitive_type=1&keywords=");
	post_data.append(skey_words);
	post_data.append("&pd=medical");

	QByteArray len1;
	len1.append(QString::number(post_data.length()));
	request.setRawHeader("Content-Length", len1);
	request.setRawHeader("Chunked", "false");

	QNetworkReply* reply = naManager->put(request, post_data);
	//QNetworkReply* reply = m_prepare_Manager->put(request, post_data);
	m_step_listen = recognize_step::prepare;
}

void record_dll::listen_upload()
{
	QNetworkRequest request;
	QNetworkAccessManager* naManager = new QNetworkAccessManager(this);
	QMetaObject::Connection connRet = QObject::connect(naManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(listen_step_finished(QNetworkReply*)));
	//QMetaObject::Connection connRet = QObject::connect(m_upload_Manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(listen_step_finished(QNetworkReply*)));
	Q_ASSERT(connRet);

	request.setUrl(QUrl("http://raasr.xfyun.cn/api/upload"));
	request.setRawHeader("Chunked", "false");

	QString sts = "", ssign = "";
	while (true)
	{
		time_t now_time = time(NULL);
		sts = QString::number(now_time);

		QString qsbefore_sign = m_xunfei_appid + sts;
		string sbefore_sign = qsbefore_sign.toLocal8Bit();
		MD5 md5;
		md5.update(sbefore_sign);
		sbefore_sign = md5.toString();

		ssign = hmacSha1(m_xunfei_secretkey.toUtf8(), QString::fromStdString(sbefore_sign).toUtf8());
		int ipos = ssign.indexOf("+");
		if (-1 == ipos)
			break;
		else
			Sleep(100);
	}

	m_upload_listen_file = new QFile(m_recognize_listen_name);
	m_upload_listen_file->open(QIODevice::ReadOnly);

	QHttpMultiPart* multi_part = new QHttpMultiPart(QHttpMultiPart::FormDataType);
	QHttpPart data_part5;
	data_part5.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/octet-stream"));
	QString qsheader = "form-data; name=\"content\"; filename=\"" + m_recognize_listen_name + "\""; //.wav
	data_part5.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(qsheader));
	data_part5.setRawHeader("Content-Transfer-Encoding", "binary");
	data_part5.setBodyDevice(m_upload_listen_file);
	multi_part->append(data_part5);

	QHttpPart data_part;
	data_part.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("text/plain; charset=UTF-8"));
	data_part.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"app_id\""));
	data_part.setRawHeader("Content-Transfer-Encoding", "8bit");
	data_part.setBody(m_xunfei_appid.toUtf8());
	multi_part->append(data_part);

	QHttpPart data_part1;
	data_part1.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("text/plain; charset=UTF-8"));
	data_part1.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"ts\""));
	data_part1.setRawHeader("Content-Transfer-Encoding", "8bit");
	data_part1.setBody(sts.toLocal8Bit());
	multi_part->append(data_part1);

	QHttpPart data_part2;
	data_part2.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("text/plain; charset=UTF-8"));
	data_part2.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"signa\""));
	data_part2.setRawHeader("Content-Transfer-Encoding", "8bit");
	data_part2.setBody(ssign.toLocal8Bit());
	multi_part->append(data_part2);

	QHttpPart data_part3;
	data_part3.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("text/plain; charset=UTF-8"));
	data_part3.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"task_id\""));
	data_part3.setRawHeader("Content-Transfer-Encoding", "8bit");
	data_part3.setBody(m_xunfei_taskid_listen.toLocal8Bit());
	multi_part->append(data_part3);

	string slice_id = m_genid.getNextSliceId();
	QHttpPart data_part4;
	data_part4.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("text/plain; charset=UTF-8"));
	data_part4.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"slice_id\""));
	data_part4.setRawHeader("Content-Transfer-Encoding", "8bit");
	data_part4.setBody(slice_id.c_str());
	multi_part->append(data_part4);

	QNetworkReply* reply = naManager->post(request, multi_part);
	//QNetworkReply* reply = m_upload_Manager->post(request, multi_part);
	m_step_listen = recognize_step::upload;
}

void record_dll::listen_merge()
{
	QNetworkRequest request;
	QNetworkAccessManager* naManager = new QNetworkAccessManager(this);
	QMetaObject::Connection connRet = QObject::connect(naManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(listen_step_finished(QNetworkReply*)));
	//m_merge_Manager = new QNetworkAccessManager(this);
	//QMetaObject::Connection connRet = QObject::connect(m_merge_Manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(listen_step_finished(QNetworkReply*)));
	Q_ASSERT(connRet);                                 

	request.setUrl(QUrl("http://raasr.xfyun.cn/api/merge"));
	request.setRawHeader("Content-Type", "application/x-www-form-urlencoded; charset=UTF-8");

	QString sts = "", ssign = "";
	while (true)
	{
		time_t now_time = time(NULL);
		sts = QString::number(now_time);

		QString qsbefore_sign = m_xunfei_appid + sts;
		string sbefore_sign = qsbefore_sign.toLocal8Bit();
		MD5 md5;
		md5.update(sbefore_sign);
		sbefore_sign = md5.toString();
		ssign = hmacSha1(m_xunfei_secretkey.toUtf8(), QString::fromStdString(sbefore_sign).toUtf8());

		int ipos = ssign.indexOf("+");
		if (-1 == ipos)
			break;
		else
			Sleep(100);
	}

	QByteArray post_data;
	post_data.append("signa=");
	post_data.append(ssign);
	post_data.append("&ts=");
	post_data.append(sts);
	post_data.append("&app_id=");
	post_data.append(m_xunfei_appid);
	post_data.append("&task_id=");
	post_data.append(m_xunfei_taskid_listen);

	QByteArray len;
	len.append(QString::number(post_data.length()));
	request.setRawHeader("Content-Length", len);
	request.setRawHeader("Chunked", "false");

	QNetworkReply* reply = naManager->put(request, post_data); 
	//QNetworkReply* reply = m_merge_Manager->put(request, post_data);
	m_step_listen = recognize_step::merge;
}

int record_dll::listen_recognize_baidu(char* pvoice, long len)
{
	m_blisten_recognize = true;

	QNetworkRequest request;
	QNetworkAccessManager* naManager = new QNetworkAccessManager(this);
	QMetaObject::Connection connRet = QObject::connect(naManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(listen_baidu_finished(QNetworkReply*)));
	Q_ASSERT(connRet);

	request.setUrl(QUrl("http://vop.baidu.com/server_api"));
	request.setRawHeader("Content-Type", "application/json");

	QByteArray qcontent(pvoice, len);
	QByteArray qafter = qcontent.toBase64();
	
	QByteArray post_data;
	post_data.append("{\"format\":\"wav\",");
	post_data.append("\"rate\":16000,");
	post_data.append("\"dev_pid\":1537,");
	post_data.append("\"channel\":1,");
	post_data.append("\"token\":\"");
	post_data.append(m_baidu_token);
	post_data.append("\",");
	post_data.append("\"cuid\":\"xtkq_user\",");
	post_data.append("\"len\":");
	post_data.append(QString::number(len).toLocal8Bit().data());
	post_data.append(",");
	post_data.append("\"speech\":\"");
	post_data.append(qafter);
	post_data.append("\"}");

	QNetworkReply* reply = naManager->put(request, post_data);

	return 0;
}
//end private functions