#include "record_dll.h"

#include <QTextCodec>

//begin public functions
//向百度提交语音识别请求,spath 为带文件名的语音文件路径，后缀为.wav/.WAV
int record_dll::recognize_baidu(QString spath)
{
	if (m_baidu_recflag)    //正在进行识别
	{
		m_plog->Trace("baidu is recognizing...");
		return -1;
	}

	if ("" == spath)
	{
		m_plog->Trace("baidu recognize path is null,please confirm!!");
		return -2;
	}

	QString suffix = spath.right(4);
	if (suffix != ".wav" && suffix != ".WAV")
	{
		m_plog->Trace("baidu recognize,suffix is not wav,WAV,please confirm!!");
		return -3;
	}

	m_vbaidu_rec_result.clear();
	//spath = "d:/1.wav";
	m_precognize_voice = new Voice(spath, m_voice_channel_num);
	assert(m_precognize_voice);
	m_plog->Trace("baidu recognize is begin!!");

	return recognize_baidu_private();
}

//获取百度的识别结果
//返回值 -1--识别还没有结束  -2--识别结束，没有识别结果 0--正确获取识别结果
int record_dll::get_result_baidu(vector<QString>& result)
{
	if (m_baidu_recflag)
	{
		m_plog->Trace("baidu recognize is not over!!");
		return -1;
	}

	if (0 == m_vbaidu_rec_result.size())
	{
		m_plog->Trace("baidu recognize result is null!!");
		return -2;
	}

	for (auto it : m_vbaidu_rec_result)
		result.push_back(it);

	return 0;
}

//如果要用百度识别，此接口必须被先调用
void record_dll::set_baidu_token(QString token)
{
	assert(token != "");
	m_baidu_token = token;
}
//end public functions

//begin slot functions
void record_dll::baidu_rec_finished(QNetworkReply* reply)
{
	QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
	if (!statusCode.isValid())
	{
		m_plog->Trace("baidu recognize fail,statusCode is invalid!!");
		return;
	}

	if (statusCode.toUInt() != 200)
	{
		m_plog->Trace("baidu recognize fail,return code is %d", statusCode.toUInt());
		return;
	}

	QNetworkReply::NetworkError err = reply->error();
	if (err != QNetworkReply::NoError)
	{
		m_plog->Trace("baidu recognize fail,err is %d", err);
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

			m_vbaidu_rec_result.push_back(qs_result);
			m_plog->Trace("baidu recognize result is %s", qs_result.toLocal8Bit().data());
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
	}//end else

	recognize_baidu_private();
}
//end slot functions

//begin private functions
int record_dll::recognize_baidu_private()
{
	int iepos = m_precognize_voice->get_epos(m_ibpos);
	if (0 == iepos)
	{
		m_baidu_recflag = false;
		m_ibpos = 44;

		delete m_precognize_voice;
		m_precognize_voice = nullptr;
		m_plog->Trace("baidu recognize is over!!");

		return 0;
	}

	QNetworkRequest request;
	QNetworkAccessManager* naManager = new QNetworkAccessManager(this);
	QMetaObject::Connection connRet = QObject::connect(naManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(baidu_rec_finished(QNetworkReply*)));
	Q_ASSERT(connRet);

	request.setUrl(QUrl("http://vop.baidu.com/server_api"));
	request.setRawHeader("Content-Type", "application/json");

	char* pvoice = new char[iepos - m_ibpos + 1];
	m_precognize_voice->get_rec_voicedata(m_ibpos, iepos, pvoice);

	int length = iepos - m_ibpos;
	QByteArray qcontent(pvoice, length);
	//qcontent.resize(length);
	//for (int i = 0; i < length; ++i)
	//	qcontent[i] = pvoice[i];

	QByteArray qafter = qcontent.toBase64();
	if (pvoice)
	{
		delete[] pvoice;
		pvoice = nullptr;
	}

	QByteArray post_data;
	post_data.append("{\"format\":\"wav\",");
	post_data.append("\"rate\":16000,");
	post_data.append("\"dev_pid\":1537,");
	post_data.append("\"channel\":1,");
	//post_data.append("\"token\":\"24.8fe58964072d84f6a7fc8bcd898850ac.2592000.1585219010.282335-18578469\",");
	post_data.append("\"token\":\"");
	post_data.append(m_baidu_token);
	post_data.append("\",");
	post_data.append("\"cuid\":\"xtkq_user\",");
	post_data.append("\"len\":");
	post_data.append(QString::number(length).toLocal8Bit().data());
	post_data.append(",");
	post_data.append("\"speech\":\"");
	post_data.append(qafter);
	post_data.append("\"}");

	QNetworkReply* reply = naManager->put(request, post_data);
	m_ibpos = iepos;
	m_baidu_recflag = true;

	return 0;
}
//end private functions