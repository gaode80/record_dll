#include "record_dll.h"
#include "lame.h"

#include <io.h>

#include <QtNetWork/QNetworkRequest>
#include <QtNetWork/QNetworkAccessManager>

record_dll::record_dll(QString basepath)
{
	m_voice_channel_num = 1;
	m_voice_sample_rate = 16000;
	m_mac_num = 1;
	m_ibpos = 44;
	m_ibpos_xunfei = 44;
	m_index_xunfei = 1;

	_audioInput1 = nullptr;
	_audioInput2 = nullptr;
    m_precognize_voice = nullptr;
	m_precognize_voice_xunfei = nullptr;
	m_timer = nullptr;

	m_brecord_flag = false;
	m_baidu_recflag = false;
	m_xunfei_recflag = false;

	m_curr_recordpath = "";
	m_baidu_token = "";
	m_recognize_temp_name = "";
	m_xunfei_appid = "";
	m_xunfei_secretkey = "";
	m_xunfei_taskid = "";

	m_vbaidu_rec_result.clear();

	m_plog = new CLog(0,basepath,"dll_log.log");

	//listen
	m_blisten_flag = false;
	m_blisten_recognize = false;
	m_blisten_write_flag = false;

	m_listen_input = nullptr;
	m_buffer_in = nullptr;
	m_listen_timer = nullptr;
	m_listen_voice = nullptr;
	m_upload_listen_file = nullptr;
	m_prevoice = nullptr;

	m_index_xunfei_listen = 1;
	m_iprovider = 2;
	m_prelen = 0;

	m_recognize_listen_name = "";
	m_xunfei_taskid_listen = "";
	m_listen_file_path = "";

	//m_prepare_Manager = new QNetworkAccessManager(this);
	//m_upload_Manager = new QNetworkAccessManager(this);
	//m_merge_Manager = new QNetworkAccessManager(this);
	//m_getresult_Manager = new QNetworkAccessManager(this);
}

record_dll::~record_dll()
{
	if (m_plog)
	{
		delete m_plog;
		m_plog = nullptr;
	}
}

//����ͨ����(��������ã�Ĭ��Ϊ1)
void record_dll::set_channelnum(int channels)
{ 
	if (channels > 2 || channels < 1)
		m_voice_channel_num = 1;
	else
		m_voice_channel_num = channels;
}

//���ò�����(֧�� 44100,16000,8000)�������������������֮һ��Ĭ��Ϊ16000
//����˽ӿڲ������ã�Ĭ��Ҳ��16000
void record_dll::set_samplerate(int sample_rate)
{
	switch (sample_rate)
	{
	case 44100:
		m_voice_sample_rate = 441000;
		break;
	case 16000:
		m_voice_sample_rate = 16000;
		break;
	case 8000:
		m_voice_sample_rate = 8000;
		break;
	default:
		m_voice_sample_rate = 16000;
		break;
	}
}

//������˵�����(Ĭ��Ϊ1)
void record_dll::set_macnum(int count)
{
	if (count > 2 || count < 1)
		m_mac_num = 1;
	else
		m_mac_num = count;
}

//�����Ƿ��¼��־��������Ĭ�ϲ���¼��־
void record_dll::set_islog(bool bflag)
{
	m_plog->set_logflag(bflag);
}

//��ʼ¼��,������ʼ����0���쳣���ظ���
//����spath Ϊ��¼���ļ�����·������׺����wav/WAV�᷵��-1
//�Ҳ���¼���豸����-2�������¼��ʱ������������С��2������-3
//������ڿ����������ܣ�����-4     ������¼��ʱ����ġ�
int record_dll::start_record(QString spath)
{
	if (m_blisten_flag)
	{
		m_plog->Trace("curr status is listening, can`t record!!");
		return -6;
	}
	if (m_brecord_flag)
	{
		m_plog->Trace("curr status is recording,please stop record before!!");
		return -1;
	}
		
	if ("" == spath)
	{
		m_plog->Trace("record path is null,please config path!!");
		return -2;
	}
		
	QString suffix = spath.right(4);
	if (suffix != ".wav" && suffix != ".WAV")
	{
		m_plog->Trace("suffix is not wav or WAV,please confirm suffix!!");
		return -3;
	}	

	QAudioFormat audioFormat;
	audioFormat.setByteOrder(QAudioFormat::LittleEndian);
	audioFormat.setCodec("audio/pcm");
	audioFormat.setSampleSize(16);
	audioFormat.setChannelCount(m_voice_channel_num);
	audioFormat.setSampleRate(m_voice_sample_rate);
	//audioFormat.setSampleSize(m_voice_bit_len);
	audioFormat.setSampleType(QAudioFormat::SignedInt);

	if (1 == m_mac_num)
	{
		QAudioDeviceInfo devInfo;
		//�ж��豸���鿴�Ƿ����
		devInfo = QAudioDeviceInfo::defaultInputDevice();
		if (devInfo.isNull())
		{
			m_plog->Trace("no find record device!!");
			return -4;
		}

		//��֧�ָ�ʽ��ʹ����ӽ���ʽ
		if (!devInfo.isFormatSupported(audioFormat))          //��ǰʹ���豸�Ƿ�֧��
			audioFormat = devInfo.nearestFormat(audioFormat); //ת��Ϊ��ӽ���ʽ

		//�ڴ��IO����
		_audioInput1 = new QAudioInput(devInfo, audioFormat, this);
		assert(_audioInput1);

		bak_voice_file(spath);

		m_outFile1.setFileName(spath); //����ԭʼ�ļ�
		m_outFile1.open(QIODevice::WriteOnly | QIODevice::Truncate);
		_audioInput1->start(&m_outFile1);
		//m_buffin = _audioInput->start();
		//connect(m_buffin, SIGNAL(readyRead()), SLOT(read_more()));
	}
	else if (2 == m_mac_num)
	{
		QAudioDeviceInfo devInfo1, devInfo2;
		QList<QAudioDeviceInfo> ql = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
		if (ql.count() == 2)
		{
			m_plog->Trace("record device fewer two!!");
			return -5;
		}
			
		devInfo1 = ql.at(0);
		devInfo2 = ql.at(1);

		//��֧�ָ�ʽ��ʹ����ӽ���ʽ
		if (!devInfo1.isFormatSupported(audioFormat))          //��ǰʹ���豸�Ƿ�֧��
			audioFormat = devInfo1.nearestFormat(audioFormat); //ת��Ϊ��ӽ���ʽ
		if (!devInfo2.isFormatSupported(audioFormat))          //��ǰʹ���豸�Ƿ�֧��
			audioFormat = devInfo2.nearestFormat(audioFormat); //ת��Ϊ��ӽ���ʽ

		//�ڴ��IO����
		_audioInput1 = new QAudioInput(devInfo1, audioFormat, this);
		assert(_audioInput1);
		_audioInput2 = new QAudioInput(devInfo2, audioFormat, this);
		assert(_audioInput2);

		bak_voice_file(spath);
		QString spath1 = "",spath2="";
		spath1 = spath + "_1.wav";
		spath2 = spath + "_2.wav";

		m_outFile1.setFileName(spath1); //����ԭʼ�ļ�
		m_outFile1.open(QIODevice::WriteOnly | QIODevice::Truncate);
		_audioInput1->start(&m_outFile1);    //���

		m_outFile2.setFileName(spath2); //����ԭʼ�ļ�
		m_outFile2.open(QIODevice::WriteOnly | QIODevice::Truncate);
		_audioInput2->start(&m_outFile2);    //����
	}

	m_brecord_flag = true;
	m_curr_recordpath = spath;
	m_plog->Trace("begin record success!!");

	return 0;
}

//ֹͣ¼��
int record_dll::end_record()
{
	if (!m_brecord_flag)
	{
		m_plog->Trace("curr no record,please start record first");
		return 0;
	}

	//���wav�ļ�ͷ
	static WAVHEADER wavHeader;
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
	//wavHeader.nRiffLength = device->size() - 8 + sizeof(WAVHEADER);
	//wavHeader.nDataLength = device->size();

	if (1 == m_mac_num)
	{
		QIODevice* device{ nullptr };
		device = &m_outFile1;

		wavHeader.nRiffLength = device->size() - 8 + sizeof(WAVHEADER);
		wavHeader.nDataLength = device->size();

		device->seek(0);
		device->write(reinterpret_cast<char*>(&wavHeader), sizeof WAVHEADER);

		_audioInput1->stop();
		m_outFile1.close();
		delete _audioInput1;
		_audioInput1 = nullptr;
	}
	else if (2 == m_mac_num)
	{
		QIODevice* device1{ nullptr };
		QIODevice* device2{ nullptr };
		device1 = &m_outFile1;
		device2 = &m_outFile2;

		device1->seek(0);
		device1->write(reinterpret_cast<char*>(&wavHeader), sizeof WAVHEADER);

		device2->seek(0);
		device2->write(reinterpret_cast<char*>(&wavHeader), sizeof WAVHEADER);

		_audioInput1->stop();
		_audioInput2->stop();
		m_outFile1.close();
		m_outFile2.close();
		delete _audioInput1;
		delete _audioInput2;
		_audioInput1 = nullptr;
		_audioInput2 = nullptr;

		QString qsinpath1 = "", qsinpath2 = "";
		qsinpath1 = m_curr_recordpath + "_1.wav";
		qsinpath2 = m_curr_recordpath + "_2.wav";
		mix_voice(qsinpath1, qsinpath2, m_curr_recordpath);

		remove(qsinpath1.toLocal8Bit().data());
		remove(qsinpath2.toLocal8Bit().data());
	}

	m_brecord_flag = false;
	m_plog->Trace("stop record success!!");

	return 0;
}

int record_dll::wav2mp3(const char* inPath, const char* outPath)
{
	if ("" == inPath || "" == outPath)
		return -1;

	int status = 0;
	lame_global_flags* gfp;
	int ret_code;
	FILE* infp;
	FILE* outfp;
	short* input_buffer;
	int input_samples;
	unsigned char* mp3_buffer;
	int mp3_bytes;

	gfp = lame_init();
	if (gfp == NULL)
	{
		status = -1;
		goto exit;
	}

	//lame_set_num_channels(gfp, 1);
	//lame_set_in_samplerate(gfp,16000);
	//lame_set_out_samplerate(gfp,16000);

	lame_set_num_channels(gfp, m_voice_channel_num);
	lame_set_in_samplerate(gfp, m_voice_sample_rate);
	lame_set_out_samplerate(gfp, m_voice_sample_rate);

	//lame_set_mode(gfp, MONO);
	// 3. ����MP3�ı��뷽ʽ
	//lame_set_VBR(gfp, vbr_default);
	//lame_set_brate(gfp, 64);

	ret_code = lame_init_params(gfp);
	if (ret_code < 0)
	{
		//printf("lame_init_params returned %d/n", ret_code);
		status = -1;
		goto close_lame;
	}

	infp = fopen(inPath, "rb");
	outfp = fopen(outPath, "wb");
	fseek(infp, 44, SEEK_SET);       //ȥ���ļ�ͷ

	input_buffer = (short*)malloc(INBUFSIZE * 2);
	mp3_buffer = (unsigned char*)malloc(MP3BUFSIZE);
	do
	{
		//input_samples = fread(input_buffer, 2, INBUFSIZE, infp); 
		if (2 == m_voice_channel_num)
		{
			input_samples = fread(input_buffer, 2, INBUFSIZE, infp);
			mp3_bytes = lame_encode_buffer_interleaved(gfp, input_buffer, input_samples / 2, mp3_buffer, MP3BUFSIZE);
		}
		else
		{
			input_samples = fread(input_buffer, 2, INBUFSIZE, infp);
			mp3_bytes = lame_encode_buffer(gfp, input_buffer, nullptr, input_samples, mp3_buffer, MP3BUFSIZE);
			//lame_encode_buffer(lameClient, (short int*)leftBuffer, (short int*)rightBuffer, (int)(readBufferSize / 2), mp3_buffer, bufferSize);

		}
 
		if (mp3_bytes < 0)
		{
			//printf("lame_encode_buffer_interleaved returned %d\n", mp3_bytes);
			status = -1;
			goto free_buffers;
		}
		else if (mp3_bytes > 0)
		{
			fwrite(mp3_buffer, 1, mp3_bytes, outfp);
		}
	} while (input_samples == INBUFSIZE);

	mp3_bytes = lame_encode_flush(gfp, mp3_buffer, sizeof(mp3_buffer));
	if (mp3_bytes > 0)
	{
		fwrite(mp3_buffer, 1, mp3_bytes, outfp);
	}

free_buffers:
	free(mp3_buffer);
	free(input_buffer);

	fclose(outfp);
	fclose(infp);
close_lame:
	lame_close(gfp);
exit:
	return status;
}


//begin private functions
void record_dll::bak_voice_file(QString filename)
{
	//QString mp3_name = filename.left(filename.length() - 4);
	//mp3_name += ".mp3";

	//if (0 == _access(mp3_name.toLocal8Bit().data(), 00))   //�ļ�����
	if (0 == _access(filename.toLocal8Bit().data(), 00))   //�ļ�����
	{
		QString bak_name = "";
		time_t now_time = time(NULL);
		tm* ptm = localtime(&now_time);

		char ch_time[32];
		memset(ch_time, 0, 32);
		sprintf(ch_time, "_%04d%02d%02d_%02d%02d%02d", \
			    ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, \
			    ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

		bak_name = filename.left(filename.length() - 4);
		bak_name += ch_time;
		bak_name += ".wav";
		rename(filename.toLocal8Bit().data(), bak_name.toLocal8Bit().data());
	}
}

void record_dll::Mix(char sourseFile[10][SIZE_AUDIO_FRAME], int number, char* objectFile)
{
	int const MAX = 32767;
	int const MIN = -32768;
	double f = 1;
	int output;
	int i = 0, j = 0;

	for (i = 0; i < SIZE_AUDIO_FRAME / 2; i++)
	{
		int temp = 0;
		for (j = 0; j < number; j++)
			temp += *(short*)(sourseFile[j] + i * 2);

		output = (int)(temp * f);
		if (output > MAX)
		{
			f = (double)MAX / (double)(output);
			output = MAX;
		}

		if (output < MIN)
		{
			f = (double)MIN / (double)(output);
			output = MIN;
		}

		if (f < 1)
			f += ((double)1 - f) / (double)32;

		*(short*)(objectFile + i * 2) = (short)output;
	}
}

void record_dll::mix_voice(QString sourcefile1, QString sourcefile2, QString destfile)
{
	FILE* fp1, * fp2, * fpm;
	fopen_s(&fp1, sourcefile1.toLocal8Bit().data(), "rb");
	fopen_s(&fp2, sourcefile2.toLocal8Bit().data(), "rb");
	fopen_s(&fpm, destfile.toLocal8Bit().data(), "wb");

	short data1, data2, date_mix;
	int ret1, ret2;
	char sourseFile[10][2];

	while (1)
	{
		ret1 = fread(&data1, 2, 1, fp1);
		ret2 = fread(&data2, 2, 1, fp2);
		*(short*)sourseFile[0] = data1;
		*(short*)sourseFile[1] = data2;
		if (ret1 > 0 && ret2 > 0) {
			Mix(sourseFile, 2, (char*)&date_mix);
			/*
			if( data1 < 0 && data2 < 0)
			date_mix = data1+data2 - (data1 * data2 / -(pow(2,16-1)-1));
			else
			date_mix = data1+data2 - (data1 * data2 / (pow(2,16-1)-1));
			*/

			//if (date_mix > pow(2, 16 - 1) || date_mix < -pow(2, 16 - 1))
			//	printf("mix error\n");
		}
		else if ((ret1 > 0) && (ret2 == 0))
			date_mix = data1;
		else if ((ret2 > 0) && (ret1 == 0))
			date_mix = data2;
		else if ((ret1 == 0) && (ret2 == 0))
			break;

		fwrite(&date_mix, 2, 1, fpm);
	}

	fclose(fp1);
	fclose(fp2);
	fclose(fpm);
}
//end private functions
