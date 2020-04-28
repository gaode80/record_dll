#include "record_dll.h"
#include "MD5.h"
#include <windows.h>

//begin public functions
int record_dll::recognize_xunfei(QString spath)
{
	if (m_xunfei_recflag)    //正在进行识别
	{
		m_plog->Trace("xunfei is recognizing...");
		return -1;
	}

	if ("" == spath)
	{
		m_plog->Trace("xunfei recognize path is null,please confirm!!");
		return -2;
	}

	QString suffix = spath.right(4);
	if (suffix != ".wav" && suffix != ".WAV")
	{
		m_plog->Trace("xunfei recognize,suffix is not wav,WAV,please confirm!!");
		return -3;
	}

	m_vxunfei_rec_result.clear();
	m_precognize_voice_xunfei = new Voice(spath, m_voice_channel_num);
	assert(m_precognize_voice_xunfei);

	m_plog->Trace("xunfei recognize is begin!!");
	prepare();
	return 0;
}

//科大讯飞的appid
void record_dll::set_xunfei_appid(QString appid)
{
	m_xunfei_appid = appid;
}

//科大讯飞的秘钥
void record_dll::set_xunfei_secretkey(QString key)
{
	m_xunfei_secretkey = key;
}

//获取讯飞识别结果
int record_dll::get_result_xunfei(vector<QString>& result)
{
	if (m_xunfei_recflag)
	{
		m_plog->Trace("xunfei recognize is not over!!");
		return -1;
	}

	if (0 == m_vxunfei_rec_result.size())
	{
		m_plog->Trace("xunfei recognize result is null!!");
		return -2;
	}

	for (auto it : m_vxunfei_rec_result)
		result.push_back(it);

	return 0;
}
//end public functions

//begin slot functions
void record_dll::step_finished(QNetworkReply* reply)
{
	// 获取http状态码
	QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
	if (!statusCode.isValid())
	{
		m_plog->Trace("xunfei recognize fail,statusCode is invalid!!");
		return;
	}
	if (statusCode.toUInt() != 200)
	{
		m_plog->Trace("xunfei recognize fail,return code is %d", statusCode.toUInt());
		return;
	}

	QNetworkReply::NetworkError err = reply->error();
	if (err != QNetworkReply::NoError)
	{
		m_plog->Trace("xunfei recognize fail,err is %d", err);
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

		switch (m_step)
		{
		case recognize_step::prepare:
		{
			if (0 == qvalue.toInt())
			{
				QJsonValue qvalue = qobj.value("data");
				m_xunfei_taskid = qvalue.toString();

				upload();
			}
			else
			{
				QJsonValue qerr_no = qobj.value("err_no");
				QJsonValue qfailed = qobj.value("failed");

				QString info = "err_no=";
				info += QString::number(qerr_no.toInt());
				info += " failed=";
				info += qfailed.toString();
				m_plog->Trace("xunfei recognize fail in prepare %s", info.toLocal8Bit().data());
			}
		}
		break;
		case recognize_step::upload:
		{
			if (m_upload_file)
			{
				m_upload_file->close();
				delete m_upload_file;
				m_upload_file = nullptr;
			}
			if (0 == qvalue.toInt())
				merge();
			else
			{
				QJsonValue qerr_no = qobj.value("err_no");
				QJsonValue qfailed = qobj.value("failed");

				QString info = "err_no=";
				info += QString::number(qerr_no.toInt());
				info += " failed=";
				info += qfailed.toString();
				m_plog->Trace("xunfei recognize fail in upload %s", info.toLocal8Bit().data());
			}
		}
		break;
		case recognize_step::merge:
		{
			if (0 == qvalue.toInt())
			{
				m_timer = new QTimer;
				//设置定时器是否为单次触发。默认为 false 多次触发   
				m_timer->setSingleShot(true);

				//启动或重启定时器, 并设置定时器时间：毫秒   
				m_timer->start(2000);

				//定时器触发信号槽   
				connect(m_timer, SIGNAL(timeout()), this, SLOT(get_result()));
			}
			else
			{
				QJsonValue qerr_no = qobj.value("err_no");
				QJsonValue qfailed = qobj.value("failed");

				QString info = "err_no=";
				info += QString::number(qerr_no.toInt());
				info += " failed=";
				info += qfailed.toString();
				m_plog->Trace("xunfei recognize fail in merge %s", info.toLocal8Bit().data());
			}
		}
		break;
		case recognize_step::get_result:
		{
			if (m_timer)
			{
				delete m_timer;
				m_timer = nullptr;
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

				QString onebest = ""; //, speaker = "";
				for (int i = 0; i < json_array.size(); ++i)
				{
					QJsonObject obj_temp = json_array.at(i).toObject();
					//speaker = obj_temp.value("speaker").toString();
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
				m_vxunfei_rec_result.push_back(qcontent);
				int ret = remove(m_recognize_temp_name.toLocal8Bit());
				prepare();

			}//end if (0 == qvalue.toInt())
			else
			{
				QJsonValue qerr_no = qobj.value("err_no");
				if (26605 == qerr_no.toInt())    //26605--任务处理中
				{
					m_timer = new QTimer;
					//设置定时器是否为单次触发。默认为 false 多次触发   
					m_timer->setSingleShot(true);

					//启动或重启定时器, 并设置定时器时间：毫秒   
					m_timer->start(1000);

					//定时器触发信号槽   
					connect(m_timer, SIGNAL(timeout()), this, SLOT(get_result()));
				}
				else
				{
					QJsonValue qerr_no = qobj.value("err_no");
					QJsonValue qfailed = qobj.value("failed");

					QString info = "err_no=";
					info += QString::number(qerr_no.toInt());
					info += " failed=";
					info += qfailed.toString();
					m_plog->Trace("xunfei get result fail %s", info.toLocal8Bit().data());
					
					//即使获取本次识别结果失败，也要进行下一次的识别
					prepare();
				}
			}
		}
			break;
		default:
			break;
		}
	}//end else
}

void record_dll::get_result()
{
	QNetworkRequest request;
	QNetworkAccessManager* naManager = new QNetworkAccessManager(this);
	QMetaObject::Connection connRet = QObject::connect(naManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(step_finished(QNetworkReply*)));
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
	post_data.append(m_xunfei_taskid);

	QByteArray len;
	len.append(QString::number(post_data.length()));
	request.setRawHeader("Content-Length", len);
	request.setRawHeader("Chunked", "false");

	QNetworkReply* reply = naManager->put(request, post_data);
	m_step = recognize_step::get_result;
}
//end slot functions

//begin private functions
void record_dll::prepare()
{
	int iepos = m_precognize_voice_xunfei->get_epos(m_ibpos_xunfei);
	if (0 == iepos)
	{
		m_xunfei_recflag = false;
		m_ibpos_xunfei = 44;

		delete m_precognize_voice_xunfei;
		m_precognize_voice_xunfei = nullptr;
		m_plog->Trace("xunfei recognize is over!!");

		return;
	}

	char* pvoice = new char[iepos - m_ibpos_xunfei + 1];
	assert(pvoice);
	m_precognize_voice_xunfei->get_rec_voicedata(m_ibpos_xunfei, iepos, pvoice);
	int length = iepos - m_ibpos_xunfei;

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
	wavHeader.nRiffLength = length - 8 + sizeof(WAVHEADER);
	wavHeader.nDataLength = length;

	m_recognize_temp_name = "./xunfei_" + QString::number(m_index_xunfei) + ".wav";
	FILE* fp1 = fopen(m_recognize_temp_name.toLocal8Bit(), "wb");
	fwrite(&wavHeader, 1, sizeof(WAVHEADER), fp1);
	fwrite(pvoice, 1, length, fp1);
	fseek(fp1, 0, SEEK_END);
	int size = ftell(fp1);
	fseek(fp1, 0, SEEK_SET);
	fclose(fp1);
	if (pvoice)
	{
		delete[]pvoice;
		pvoice = nullptr;
	}

	++m_index_xunfei;
	if (10000 == m_index_xunfei)
		m_index_xunfei = 1;

	QNetworkRequest request;
	QNetworkAccessManager* naManager = new QNetworkAccessManager(this);
	QMetaObject::Connection connRet = QObject::connect(naManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(step_finished(QNetworkReply*)));
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
	post_data.append(m_recognize_temp_name);
	//post_data.append(".wav");
	//post_data.append("&has_seperate=true"); 
	//post_data.append("&has_participle=true");
	//post_data.append("&max_alternatives=3");

	QString skey_words = "";
	//skey_words = "姓名,性别,民族,出生年月,年龄,常住地址,联系电话,婚姻状况,离婚史,文化程度,职业,博士,硕士,大学,高中,初中,农民,工人,教师,公务员,是否在孕期,月经,状况,周期,生育历史,是否规律,汉族,已婚,未婚,是,否,有,无,次,男,女,年,岁,学生,医护人员,家庭主妇,公司职员,服务生,月,日";
	skey_words = QString::fromLocal8Bit("左侧乳头,左侧皮肤,左侧溢液,左侧腺体,左侧副乳,左侧疼痛,右侧乳头,右侧皮肤,右侧溢液,右侧腺体,右侧副乳,右侧疼痛,正常,上抬,凹陷,糜烂,酒窝征,橘皮样,兼有,单管,多管,血性,黄色,乳样,清水样,条絮状,局限性增厚,肿块,结节性增厚,有,无,钝痛,经常性疼痛,经期疼痛,胀痛,锐痛,否");
	post_data.append("&has_sensitive=true&sensitive_type=1&keywords=");
	post_data.append(skey_words);
	post_data.append("&pd=medical");

	QByteArray len;
	len.append(QString::number(post_data.length()));
	request.setRawHeader("Content-Length", len);
	request.setRawHeader("Chunked", "false");

	QNetworkReply* reply = naManager->put(request, post_data);
	m_ibpos_xunfei = iepos;
	m_xunfei_recflag = true;
	m_step = recognize_step::prepare;
}

void record_dll::upload()
{
	QNetworkRequest request;
	QNetworkAccessManager* naManager = new QNetworkAccessManager(this);
	QMetaObject::Connection connRet = QObject::connect(naManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(step_finished(QNetworkReply*)));
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

	m_upload_file = new QFile(m_recognize_temp_name);
	m_upload_file->open(QIODevice::ReadOnly);

	QHttpMultiPart* multi_part = new QHttpMultiPart(QHttpMultiPart::FormDataType);
	QHttpPart data_part5;
	data_part5.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/octet-stream"));
	QString qsheader = "form-data; name=\"content\"; filename=\"" + m_recognize_temp_name +"\""; //.wav
	data_part5.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(qsheader));
	data_part5.setRawHeader("Content-Transfer-Encoding", "binary");
	data_part5.setBodyDevice(m_upload_file);
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
	data_part3.setBody(m_xunfei_taskid.toLocal8Bit());
	multi_part->append(data_part3);

	string slice_id = m_genid.getNextSliceId();
	QHttpPart data_part4;
	data_part4.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("text/plain; charset=UTF-8"));
	data_part4.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"slice_id\""));
	data_part4.setRawHeader("Content-Transfer-Encoding", "8bit");
	data_part4.setBody(slice_id.c_str());
	multi_part->append(data_part4);

	QNetworkReply* reply = naManager->post(request, multi_part);
	m_step = recognize_step::upload;
}

void record_dll::merge()
{
	QNetworkRequest request;
	QNetworkAccessManager* naManager = new QNetworkAccessManager(this);
	QMetaObject::Connection connRet = QObject::connect(naManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(step_finished(QNetworkReply*)));
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
	post_data.append(m_xunfei_taskid);

	QByteArray len;
	len.append(QString::number(post_data.length()));
	request.setRawHeader("Content-Length", len);
	request.setRawHeader("Chunked", "false");

	QNetworkReply* reply = naManager->put(request, post_data);
	m_step = recognize_step::merge;
}

QString record_dll::hmacSha1(QByteArray key, QByteArray baseString)
{
	int blockSize = 64; // HMAC-SHA-1 block size, defined in SHA-1 standard

	if (key.length() > blockSize)
	{
		// if key is longer than block size (64), reduce key length with SHA-1 compression
		key = QCryptographicHash::hash(key, QCryptographicHash::Sha1);
	}

	QByteArray innerPadding(blockSize, char(0x36)); // initialize inner padding with char"6"
	QByteArray outerPadding(blockSize, char(0x5c)); // initialize outer padding with char"/"

	// ascii characters 0x36 ("6") and 0x5c ("/") are selected because they have large

	// Hamming distance (http://en.wikipedia.org/wiki/Hamming_distance)

	for (int i = 0; i < key.length(); i++)
	{
		innerPadding[i] = innerPadding[i] ^ key.at(i); // XOR operation between every byte in key and innerpadding, of key length
		outerPadding[i] = outerPadding[i] ^ key.at(i); // XOR operation between every byte in key and outerpadding, of key length
	}

	// result = hash ( outerPadding CONCAT hash ( innerPadding CONCAT baseString ) ).toBase64

	QByteArray total = outerPadding;
	QByteArray part = innerPadding;

	part.append(baseString);
	total.append(QCryptographicHash::hash(part, QCryptographicHash::Sha1));
	QByteArray hashed = QCryptographicHash::hash(total, QCryptographicHash::Sha1);

	return hashed.toBase64();
}
//end private functions