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

	//原地址
	const char* inUrl = "rtsp://admin:admin888@192.168.1.64:554/h264/ch1/main/av_stream";// "http://ivi.bupt.edu.cn/hls/cctv1hd.m3u8";
	//nginx-rtmp 直播服务器rtmp推流URL
	const char* outUrl = rtmpUrl.c_str();// "rtmp://192.168.1.115/live";
	//注册所有的编解码器
	//avcodec_register_all();
	//注册所有的封装器
	//av_register_all();
	//注册所有网络协议
	avformat_network_init();
	VideoCapture cam;
	Mat frame;
	Mat imageRGB[3];
	namedWindow("video");
	//像素格式转换上下文
	SwsContext* vsc = NULL;
	//输出的数据结构 存储非压缩的数据（视频对应RGB/YUV像素数据，音频对应PCM采样数据）
	AVFrame* yuv = NULL;
	//编码器上下文
	AVCodecContext* vc = NULL;
	//rtmp flv 封装器
	AVFormatContext* ic = NULL;
	try
	{   ////////////////////////////////////////////////////////////////
		/// 1 使用opencv打开源视频流
		cam.open(inUrl);
		if (!cam.isOpened())
		{
			throw exception("cam open failed!");
		}
		cout << inUrl << " cam open success" << endl;
		//获取视频帧的尺寸
		int inWidth = cam.get(CAP_PROP_FRAME_WIDTH);
		int inHeight = cam.get(CAP_PROP_FRAME_HEIGHT);
		int fps = cam.get(CAP_PROP_FPS);
		///2 初始化格式转换上下文
		vsc = sws_getCachedContext(vsc,
			inWidth, inHeight, AV_PIX_FMT_BGR24,     //源宽、高、像素格式
			inWidth, inHeight, AV_PIX_FMT_YUV420P,//目标宽、高、像素格式
			SWS_BICUBIC,  // 尺寸变化使用算法
			0, 0, 0
		);
		if (!vsc)
		{
			throw exception("sws_getCachedContext failed!");
		}
		///3 初始化输出的数据结构。未压缩的数据
		yuv = av_frame_alloc();
		yuv->format = AV_PIX_FMT_YUV420P;
		yuv->width = inWidth;
		yuv->height = inHeight;
		yuv->pts = 0;
		//分配yuv空间
		int ret = av_frame_get_buffer(yuv, 32);
		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}
		///4 初始化编码上下文
		//a 找到编码器
		AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
		if (!codec)
		{
			throw exception("Can`t find h264 encoder!");
		}
		//b 创建编码器上下文
		vc = avcodec_alloc_context3(codec);
		if (!vc)
		{
			throw exception("avcodec_alloc_context3 failed!");
		}
		//c 配置编码器参数
		vc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER; //全局参数
		vc->codec_id = codec->id;
		vc->thread_count = 8;
		vc->bit_rate = 50 * 1024 * 8;//压缩后每秒视频的bit位大小 50kB
		vc->width = inWidth;
		vc->height = inHeight;
		vc->time_base = { 1,fps };
		vc->framerate = { fps,1 };
		//画面组的大小，多少帧一个关键帧
		vc->gop_size = 50;
		vc->max_b_frames = 0;
		vc->pix_fmt = AV_PIX_FMT_YUV420P;
		//d 打开编码器上下文
		ret = avcodec_open2(vc, 0, 0);
		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}
		cout << "avcodec_open2 success!" << endl;
		///5 输出封装器和视频流配置
		//a 创建输出封装器上下文
		ret = avformat_alloc_output_context2(&ic, 0, "flv", outUrl);
		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}
		//b 添加视频流   视音频流对应的结构体，用于视音频编解码。
		AVStream* vs = avformat_new_stream(ic, NULL);
		if (!vs)
		{
			throw exception("avformat_new_stream failed");
		}
		//附加标志，这个一定要设置
		vs->codecpar->codec_tag = 0;
		//从编码器复制参数
		avcodec_parameters_from_context(vs->codecpar, vc);
		av_dump_format(ic, 0, outUrl, 1);
		///打开rtmp 的网络输出IO  AVIOContext：输入输出对应的结构体，用于输入输出（读写文件，RTMP协议等）。
		ret = avio_open(&ic->pb, outUrl, AVIO_FLAG_WRITE);
		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}
		//写入封装头
		ret = avformat_write_header(ic, NULL);
		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}
		//存储压缩数据（视频对应H.264等码流数据，音频对应AAC/MP3等码流数据）
		AVPacket pack;
		memset(&pack, 0, sizeof(pack));
		int vpts = 0;
		for (;;)
		{
			///读取rtsp视频帧，解码视频帧
			if (!cam.grab())
			{
				continue;
			}
			///yuv转换为rgb
			if (!cam.retrieve(frame))
			{
				continue;
			}
			imshow("video", frame);

			//6：00——18：00转流;18：00——6：00增强
#pragma region AlarmClock
			//time_t now = time(0);

			///*cout << "1970 到目前经过秒数:" << now << endl;*/

			//tm* ltm = localtime(&now);

			//// 输出 tm 结构的各个组成部分
			///*cout << "年: " << 1900 + ltm->tm_year << endl;
			//cout << "月: " << 1 + ltm->tm_mon << endl;
			//cout << "日: " << ltm->tm_mday << endl;*/
			//cout << "时间: " << ltm->tm_hour << ":";
			//cout << ltm->tm_min << ":";
			//cout << ltm->tm_sec << endl;
			//if (ltm->tm_hour == 6 && ltm->tm_min == 0 && ltm->tm_sec == 0) {
			//	cout << "6 OK" << endl;
			//}
			//if (ltm->tm_hour == 18 && ltm->tm_min == 0 && ltm->tm_sec == 0) {
			//	cout << "18 OK" << endl;
			//}
#pragma endregion
#pragma region imageEnhance 图像增强

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
			//输入的数据结构
			uint8_t* indata[AV_NUM_DATA_POINTERS] = { 0 };
			//indata[0] bgrbgrbgr
			//plane indata[0] bbbbb indata[1]ggggg indata[2]rrrrr 
			indata[0] = frame.data;
			int insize[AV_NUM_DATA_POINTERS] = { 0 };
			//一行（宽）数据的字节数
			insize[0] = frame.cols * frame.elemSize();
			int h = sws_scale(vsc, indata, insize, 0, frame.rows, //源数据
				yuv->data, yuv->linesize);
			if (h <= 0)
			{
				continue;
			}
			//cout << h << " " << flush;
			///h264编码
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
			//计算pts dts和duration。
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