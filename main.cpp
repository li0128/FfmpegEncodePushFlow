#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
extern "C"
{
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avformat.lib")
//#pragma comment(lib,"opencv_world410d.lib")
#pragma comment(lib,"opencv_world410.lib")

#include <winsock2.h>
#include <iphlpapi.h>
#include <iostream>
#pragma comment(lib, "IPHLPAPI.lib")

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))

using namespace std;
using namespace cv;
int main(int argc, char* argv[])
{
#pragma region getIP
	PIP_ADAPTER_INFO pAdapterInfo;
	//PIP_ADAPTER_INFO pAdapter = NULL;

	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);

	pAdapterInfo = (IP_ADAPTER_INFO*)MALLOC(sizeof(IP_ADAPTER_INFO));

	GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);

	string localIP = pAdapterInfo->IpAddressList.IpAddress.String;
	string rtmpUrl = "rtmp://" + localIP + "/live";
	cout << rtmpUrl << endl;

#pragma region endregion

	//ԭ��ַ
	const char* inUrl = "rtsp://admin:admin888@192.168.1.64:554/h264/ch1/main/av_stream";// "http://ivi.bupt.edu.cn/hls/cctv1hd.m3u8";
	//nginx-rtmp ֱ��������rtmp����URL
	const char* outUrl = rtmpUrl.c_str();// "rtmp://192.168.1.115/live";
	//ע�����еı������
	//avcodec_register_all();
	//ע�����еķ�װ��
	//av_register_all();
	//ע����������Э��
	avformat_network_init();
	VideoCapture cam;
	Mat frame;
	Mat imageRGB[3];
	namedWindow("video");
	//���ظ�ʽת��������
	SwsContext* vsc = NULL;
	//��������ݽṹ �洢��ѹ�������ݣ���Ƶ��ӦRGB/YUV�������ݣ���Ƶ��ӦPCM�������ݣ�
	AVFrame* yuv = NULL;
	//������������
	AVCodecContext* vc = NULL;
	//rtmp flv ��װ��
	AVFormatContext* ic = NULL;
	try
	{   ////////////////////////////////////////////////////////////////
		/// 1 ʹ��opencv��Դ��Ƶ��
		cam.open(inUrl);
		if (!cam.isOpened())
		{
			throw exception("cam open failed!");
		}
		cout << inUrl << " cam open success" << endl;
		//��ȡ��Ƶ֡�ĳߴ�
		int inWidth = cam.get(CAP_PROP_FRAME_WIDTH);
		int inHeight = cam.get(CAP_PROP_FRAME_HEIGHT);
		int fps = cam.get(CAP_PROP_FPS);
		///2 ��ʼ����ʽת��������
		vsc = sws_getCachedContext(vsc,
			inWidth, inHeight, AV_PIX_FMT_BGR24,     //Դ���ߡ����ظ�ʽ
			inWidth, inHeight, AV_PIX_FMT_YUV420P,//Ŀ����ߡ����ظ�ʽ
			SWS_BICUBIC,  // �ߴ�仯ʹ���㷨
			0, 0, 0
		);
		if (!vsc)
		{
			throw exception("sws_getCachedContext failed!");
		}
		///3 ��ʼ����������ݽṹ��δѹ��������
		yuv = av_frame_alloc();
		yuv->format = AV_PIX_FMT_YUV420P;
		yuv->width = inWidth;
		yuv->height = inHeight;
		yuv->pts = 0;
		//����yuv�ռ�
		int ret = av_frame_get_buffer(yuv, 32);
		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}
		///4 ��ʼ������������
		//a �ҵ�������
		AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
		if (!codec)
		{
			throw exception("Can`t find h264 encoder!");
		}
		//b ����������������
		vc = avcodec_alloc_context3(codec);
		if (!vc)
		{
			throw exception("avcodec_alloc_context3 failed!");
		}
		//c ���ñ���������
		vc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER; //ȫ�ֲ���
		vc->codec_id = codec->id;
		vc->thread_count = 8;
		vc->bit_rate = 50 * 1024 * 8;//ѹ����ÿ����Ƶ��bitλ��С 50kB
		vc->width = inWidth;
		vc->height = inHeight;
		vc->time_base = { 1,fps };
		vc->framerate = { fps,1 };
		//������Ĵ�С������֡һ���ؼ�֡
		vc->gop_size = 50;
		vc->max_b_frames = 0;
		vc->pix_fmt = AV_PIX_FMT_YUV420P;
		//d �򿪱�����������
		ret = avcodec_open2(vc, 0, 0);
		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}
		cout << "avcodec_open2 success!" << endl;
		///5 �����װ������Ƶ������
		//a ���������װ��������
		ret = avformat_alloc_output_context2(&ic, 0, "flv", outUrl);
		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}
		//b �����Ƶ��   ����Ƶ����Ӧ�Ľṹ�壬��������Ƶ����롣
		AVStream* vs = avformat_new_stream(ic, NULL);
		if (!vs)
		{
			throw exception("avformat_new_stream failed");
		}
		//���ӱ�־�����һ��Ҫ����
		vs->codecpar->codec_tag = 0;
		//�ӱ��������Ʋ���
		avcodec_parameters_from_context(vs->codecpar, vc);
		av_dump_format(ic, 0, outUrl, 1);
		///��rtmp ���������IO  AVIOContext�����������Ӧ�Ľṹ�壬���������������д�ļ���RTMPЭ��ȣ���
		ret = avio_open(&ic->pb, outUrl, AVIO_FLAG_WRITE);
		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}
		//д���װͷ
		ret = avformat_write_header(ic, NULL);
		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}
		//�洢ѹ�����ݣ���Ƶ��ӦH.264���������ݣ���Ƶ��ӦAAC/MP3���������ݣ�
		AVPacket pack;
		memset(&pack, 0, sizeof(pack));
		int vpts = 0;
		for (;;)
		{
			///��ȡrtsp��Ƶ֡��������Ƶ֡
			if (!cam.grab())
			{
				continue;
			}
			///yuvת��Ϊrgb
			if (!cam.retrieve(frame))
			{
				continue;
			}
			imshow("video", frame);

			//6��00����18��00ת��;18��00����6��00��ǿ
#pragma region AlarmClock
			//time_t now = time(0);

			///*cout << "1970 ��Ŀǰ��������:" << now << endl;*/

			//tm* ltm = localtime(&now);

			//// ��� tm �ṹ�ĸ�����ɲ���
			///*cout << "��: " << 1900 + ltm->tm_year << endl;
			//cout << "��: " << 1 + ltm->tm_mon << endl;
			//cout << "��: " << ltm->tm_mday << endl;*/
			//cout << "ʱ��: " << ltm->tm_hour << ":";
			//cout << ltm->tm_min << ":";
			//cout << ltm->tm_sec << endl;
			//if (ltm->tm_hour == 6 && ltm->tm_min == 0 && ltm->tm_sec == 0) {
			//	cout << "6 OK" << endl;
			//}
			//if (ltm->tm_hour == 18 && ltm->tm_min == 0 && ltm->tm_sec == 0) {
			//	cout << "18 OK" << endl;
			//}
#pragma endregion
#pragma region imageEnhance ͼ����ǿ

			split(frame, imageRGB);
			for (int i = 0; i < 3; i++)
			{
				equalizeHist(imageRGB[i], imageRGB[i]);
			}
			//imageRGB[0] = imageRGB[0] + Mat::ones(imageRGB[0].size(), CV_8UC1);
			merge(imageRGB, 3, frame);
#pragma endregion
			waitKey(1);
			///rgb to yuv
			//��������ݽṹ
			uint8_t* indata[AV_NUM_DATA_POINTERS] = { 0 };
			//indata[0] bgrbgrbgr
			//plane indata[0] bbbbb indata[1]ggggg indata[2]rrrrr 
			indata[0] = frame.data;
			int insize[AV_NUM_DATA_POINTERS] = { 0 };
			//һ�У������ݵ��ֽ���
			insize[0] = frame.cols * frame.elemSize();
			int h = sws_scale(vsc, indata, insize, 0, frame.rows, //Դ����
				yuv->data, yuv->linesize);
			if (h <= 0)
			{
				continue;
			}
			//cout << h << " " << flush;
			///h264����
			yuv->pts = vpts;
			vpts++;
			ret = avcodec_send_frame(vc, yuv);
			if (ret != 0)
				continue;
			ret = avcodec_receive_packet(vc, &pack);
			if (ret != 0 || pack.size > 0)
			{
				//cout << "*" << pack.size << flush;
			}
			else
			{
				continue;
			}
			//����pts dts��duration��
			pack.pts = av_rescale_q(pack.pts, vc->time_base, vs->time_base);
			pack.dts = av_rescale_q(pack.dts, vc->time_base, vs->time_base);
			pack.duration = av_rescale_q(pack.duration, vc->time_base, vs->time_base);
			ret = av_interleaved_write_frame(ic, &pack);
			if (ret == 0)
			{
				cout << "#" << flush;
			}
		}
	}
	catch (exception& ex)
	{
		if (cam.isOpened())
			cam.release();
		if (vsc)
		{
			sws_freeContext(vsc);
			vsc = NULL;
		}
		if (vc)
		{
			avio_closep(&ic->pb);
			avcodec_free_context(&vc);
		}
		cerr << ex.what() << endl;
	}
	getchar();
	return 0;
}