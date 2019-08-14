// ConsoleApplication20.cpp : �������̨Ӧ�ó������ڵ㡣
//

//#include "stdafx.h"


#include <opencv2/opencv.hpp>
#include <iostream>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <windows.h>
#include <ctime>

#pragma comment(lib,"opencv_world347.lib")

using namespace cv;
using namespace std;


void FastFilter(IplImage* img, double sigma)
{
	int filter_size;


	if (sigma > 300)
		sigma = 300;

	filter_size = (int)floor(sigma * 6) / 2;
	filter_size = filter_size * 2 + 1;

	// If 3 sigma is less than a pixel, why bother (ie sigma < 2/3)
	// ���3 * sigmaС��һ�����أ���ֱ���˳�
	if (filter_size < 3) return;

	// Filter, or downsample and recurse
	// ����ʽ��(1) �˲�  (2) ��˹�⻬����  (3) �ݹ鴦���˲�����С
	if (filter_size < 10)
	{
		cvSmooth(img, img, CV_GAUSSIAN, filter_size, filter_size);

	}
	else
	{
		if (img->width < 2 || img->height < 2) return;
		IplImage* sub_img = cvCreateImage(cvSize(img->width / 2, img->height / 2), img->depth, img->nChannels);
		cvPyrDown(img, sub_img);
		FastFilter(sub_img, sigma / 2.0);
		cvResize(sub_img, img, CV_INTER_LINEAR);
		cvReleaseImage(&sub_img);
	}
}

void MultiScaleRetinexCR(IplImage* img, vector<double> weights, vector<double> sigmas,
	int gain, int offset, double restoration_factor, double color_gain)
{
	int i;
	double weight;
	int scales = sigmas.size();
	IplImage* A, * B, * C, * fA, * fB, * fC, * fsA, * fsB, * fsC, * fsD, * fsE, * fsF;

	// Initialize temp images
	// ��ʼ������ͼ��
	fA = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_32F, img->nChannels);
	fB = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_32F, img->nChannels);
	fC = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_32F, img->nChannels);
	fsA = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_32F, 1);
	fsB = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_32F, 1);
	fsC = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_32F, 1);
	fsD = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_32F, 1);
	fsE = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_32F, 1);
	fsF = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_32F, 1);

	// Compute log image
	// �������ͼ��
	cvConvert(img, fB);
	cvLog(fB, fA);

	// Normalize according to given weights
	// ����Ȩ�ع�һ��
	for (i = 0, weight = 0; i < scales; i++)
		weight += weights[i];

	if (weight != 1.0) cvScale(fA, fA, weight);

	// Filter at each scale
	// ���߶��Ͻ����˲�����
	for (i = 0; i < scales; i++)
	{
		A = cvCloneImage(img);
		FastFilter(A, sigmas[i]);

		cvConvert(A, fB);
		cvLog(fB, fC);
		cvReleaseImage(&A);

		// Compute weighted difference
		// ����Ȩ�غ���ͼ��֮��
		cvScale(fC, fC, weights[i]);
		cvSub(fA, fC, fA);
	}

	// Color restoration
	// ��ɫ�޸�
	if (img->nChannels > 1) {
		A = cvCreateImage(cvSize(img->width, img->height), img->depth, 1);
		B = cvCreateImage(cvSize(img->width, img->height), img->depth, 1);
		C = cvCreateImage(cvSize(img->width, img->height), img->depth, 1);

		// Divide image into channels, convert and store sum
		// ��ͼ��ָ�Ϊ����ͨ��������ת��Ϊ�����ͣ����洢ͨ������֮��
		cvSplit(img, A, B, C, NULL);
		cvConvert(A, fsA);
		cvConvert(B, fsB);
		cvConvert(C, fsC);

		cvReleaseImage(&A);
		cvReleaseImage(&B);
		cvReleaseImage(&C);

		// Sum components
		// ���
		cvAdd(fsA, fsB, fsD);
		cvAdd(fsD, fsC, fsD);

		// Normalize weights
		// ��Ȩ�ؾ����һ��
		cvDiv(fsA, fsD, fsA, restoration_factor);
		cvDiv(fsB, fsD, fsB, restoration_factor);
		cvDiv(fsC, fsD, fsC, restoration_factor);

		cvConvertScale(fsA, fsA, 1, 1);
		cvConvertScale(fsB, fsB, 1, 1);
		cvConvertScale(fsC, fsC, 1, 1);

		// Log weights
		// ��Ȩ�ؾ��������
		cvLog(fsA, fsA);
		cvLog(fsB, fsB);
		cvLog(fsC, fsC);

		// Divide retinex image, weight accordingly and recombine
		// ��Retinexͼ���з�Ϊ�������飬����Ȩ�غ���ɫ�����������
		cvSplit(fA, fsD, fsE, fsF, NULL);

		cvMul(fsD, fsA, fsD, color_gain);
		cvMul(fsE, fsB, fsE, color_gain);
		cvMul(fsF, fsC, fsF, color_gain);

		cvMerge(fsD, fsE, fsF, NULL, fA);
	}

	// Restore
	// �ָ�ͼ��
	cvConvertScale(fA, img, gain, offset);

	// Release temp images
	// �ͷŻ���ͼ��
	cvReleaseImage(&fA);
	cvReleaseImage(&fB);
	cvReleaseImage(&fC);
	cvReleaseImage(&fsA);
	cvReleaseImage(&fsB);
	cvReleaseImage(&fsC);
	cvReleaseImage(&fsD);
	cvReleaseImage(&fsE);
	cvReleaseImage(&fsF);
}


void MultiScaleRetinexCRmat(Mat src, Mat& dst, vector<double> weights, vector<double> sigmas,
	int gain, int offset, double restoration_factor, double color_gain)
{
	IplImage tmp_ipl;
	tmp_ipl = IplImage(src);
	MultiScaleRetinexCR(&tmp_ipl, weights, sigmas, gain, offset, restoration_factor, color_gain);
	dst = cvarrToMat(&tmp_ipl);
}

bool histMatch_Value(Mat matSrc, Mat matDst, Mat& matRet)
{
	if (matSrc.empty() || matDst.empty() || 1 != matSrc.channels() || 1 != matDst.channels())
		return false;


	int nHeight;
	int nWidth;
	int nDstPixNum;
	int nSrcPixNum;
	int arraySrcNum[256] = { 0 };                // Դͼ����Ҷ�ͳ�Ƹ���
	int arrayDstNum[256] = { 0 };                // Ŀ��ͼ����Ҷ�ͳ�Ƹ���
	double arraySrcProbability[256] = { 0.0 };   // Դͼ������Ҷȸ���
	double arrayDstProbability[256] = { 0.0 };   // Ŀ��ͼ������Ҷȸ���
	memset(arraySrcNum, 0, sizeof(arraySrcNum));
	memset(arrayDstNum, 0, sizeof(arrayDstNum));
	memset(arraySrcProbability, 0, sizeof(arraySrcProbability));
	memset(arrayDstProbability, 0, sizeof(arrayDstProbability));

	nHeight = matDst.rows;
	nWidth = matDst.cols;
	nDstPixNum = nHeight * nWidth;

	for (int j = 0; j < nHeight; j++)
	{
		for (int i = 0; i < nWidth; i++)
		{
			arrayDstNum[matDst.at<uchar>(j, i)]++;
		}
	}
	for (int i = 0; i < 256; i++)
	{
		arrayDstProbability[i] = (double)(1.0 * arrayDstNum[i] / nDstPixNum);
	}

	nHeight = matSrc.rows;
	nWidth = matSrc.cols;
	nSrcPixNum = nHeight * nWidth;
	for (int j = 0; j < nHeight; j++)
	{
		for (int i = 0; i < nWidth; i++)
		{
			arraySrcNum[matSrc.at<uchar>(j, i)]++;
		}
	}
	// �������
	for (int i = 0; i < 256; i++)
	{
		arraySrcProbability[i] = (double)(1.0 * arraySrcNum[i] / nSrcPixNum);
	}

	// ����ֱ��ͼ����ӳ��
	int L = 256;
	int arraySrcMap[256] = { 0 };
	int arrayDstMap[256] = { 0 };
	for (int i = 0; i < L; i++)
	{
		double dSrcTemp = 0.0;
		double dDstTemp = 0.0;
		for (int j = 0; j <= i; j++)
		{
			dSrcTemp += arraySrcProbability[j];
			dDstTemp += arrayDstProbability[j];
		}
		arraySrcMap[i] = (int)((L - 1) * dSrcTemp + 0.5);// ��ȥ1��Ȼ����������
		arrayDstMap[i] = (int)((L - 1) * dDstTemp + 0.5);// ��ȥ1��Ȼ����������
	}
	// ����ֱ��ͼƥ��Ҷ�ӳ��
	int grayMatchMap[256] = { 0 };
	for (int i = 0; i < L; i++) // i��ʾԴͼ��Ҷ�ֵ
	{
		int nValue = 0;    // ��¼ӳ���ĻҶ�ֵ
		int nValue_1 = 0;  // ��¼���û���ҵ���Ӧ�ĻҶ�ֵʱ����ӽ��ĻҶ�ֵ
		int k = 0;
		int nTemp = arraySrcMap[i];
		for (int j = 0; j < L; j++) // j��ʾĿ��ͼ��Ҷ�ֵ
		{
			// ��Ϊ����ɢ����£�֮��ͼ���⻯�����Ѿ������ϸ񵥵����ˣ�
			// ���Է��������ܳ���һ�Զ�������������������ƽ����
			if (nTemp == arrayDstMap[j])
			{
				nValue += j;
				k++;
			}
			if (nTemp < arrayDstMap[j])
			{
				nValue_1 = j;
				break;
			}
		}
		if (k == 0)// ��ɢ����£�������������Щֵ�Ҳ������Ӧ�ģ�����ȥ��ӽ���һ��ֵ
		{
			nValue = nValue_1;
			k = 1;
		}
		grayMatchMap[i] = nValue / k;
	}
	// ������ͼ��
	matRet = Mat::zeros(nHeight, nWidth, CV_8UC1);
	for (int j = 0; j < nHeight; j++)
	{
		for (int i = 0; i < nWidth; i++)
		{
			matRet.at<uchar>(j, i) = grayMatchMap[matSrc.at<uchar>(j, i)];
		}
	}
	return true;
}
int generatehistogram(Mat& matSrc, int arraySrcMap[3][256])
{
	int nHeight;
	int nWidth;
	int nDstPixNum;
	int arraySrcNum[256] = { 0 };                // Դͼ����Ҷ�ͳ�Ƹ���
	double arraySrcProbability[3][256];
	memset(arraySrcProbability, 0, sizeof(arraySrcProbability));
	memset(arraySrcMap, 0, sizeof(arraySrcMap));
	nHeight = matSrc.rows;
	nWidth = matSrc.cols;
	nDstPixNum = nHeight * nWidth;
	for (int z = 0; z < 3; z++)
	{
		memset(arraySrcNum, 0, sizeof(arraySrcNum));
		for (int j = 0; j < nHeight; j++)
		{
			for (int i = 0; i < nWidth; i++)
			{
				arraySrcNum[matSrc.at<Vec3b>(j, i)[z]]++;
			}
		}
		for (int i = 0; i < 256; i++)
		{
			arraySrcProbability[z][i] = (double)(1.0 * arraySrcNum[i] / nDstPixNum);
		}
		for (int i = 0; i < 256; i++)
		{
			double dSrcTemp = 0.0;
			double dDstTemp = 0.0;
			for (int j = 0; j <= i; j++)
			{
				dSrcTemp += arraySrcProbability[z][j];

			}
			arraySrcMap[z][i] = (int)(255 * dSrcTemp + 0.5);// ��ȥ1��Ȼ����������
		}
	}
	return 0;
}
int histogram_Eq(int arraySrcMap[3][256], int arrayDstMap[3][256], Mat& matDst)
{
	int nHeight;
	int nWidth;
	int nDstPixNum;

	nHeight = matDst.rows;
	nWidth = matDst.cols;
	nDstPixNum = nHeight * nWidth;
	int L = 256;
	for (int z = 0; z < 3; z++)
	{
		int grayMatchMap[256] = { 0 };
		for (int i = 0; i < L; i++)
		{
			int nValue = 0;
			int nValue_1 = 0;
			int k = 0;
			int nTemp = arraySrcMap[z][i];
			for (int j = 0; j < L; j++) // j��ʾĿ��ͼ��Ҷ�ֵ
			{
				// ��Ϊ����ɢ����£�֮��ͼ���⻯�����Ѿ������ϸ񵥵����ˣ�
				// ���Է��������ܳ���һ�Զ�������������������ƽ����
				if (nTemp == arrayDstMap[z][j])
				{
					nValue += j;
					k++;
				}
				if (nTemp < arrayDstMap[z][j])
				{
					nValue_1 = j;
					break;
				}
			}
			if (k == 0)// ��ɢ����£�������������Щֵ�Ҳ������Ӧ�ģ�����ȥ��ӽ���һ��ֵ
			{
				nValue = nValue_1;
				k = 1;
			}
			grayMatchMap[i] = nValue / k;
		}

		for (int j = 0; j < nHeight; j++)
		{
			for (int i = 0; i < nWidth; i++)
			{
				matDst.at<Vec3b>(j, i)[z] = grayMatchMap[matDst.at<Vec3b>(j, i)[z]];
			}
		}
	}

	return 0;
}
int histogram_Matching(Mat& matSrc, Mat& matDst)
{


	Mat srcBGR[3];
	Mat dstBGR[3];
	Mat retBGR[3];
	split(matSrc, srcBGR);
	split(matDst, dstBGR);

	histMatch_Value(srcBGR[0], dstBGR[0], retBGR[0]);
	histMatch_Value(srcBGR[1], dstBGR[1], retBGR[1]);
	histMatch_Value(srcBGR[2], dstBGR[2], retBGR[2]);

	merge(retBGR, 3, matSrc);
	return 0;
}

int main()
{
	const char* inUrl = "rtsp://admin:admin888@192.168.1.64:554/h264/ch1/main/av_stream";
	char x1[200] = "C:\\Users\\XnoZero\\Desktop\\matlab\\test\\data\\2.mp4";
	char x2[200] = "C:\\Users\\XnoZero\\Desktop\\matlab\\test\\data\\out.avi";
	VideoCapture capture(inUrl);
	VideoWriter out;
	out.open(x2, CV_FOURCC('X', 'V', 'I', 'D'), capture.get(CV_CAP_PROP_FPS), Size(capture.get(CV_CAP_PROP_FRAME_WIDTH), capture.get(CV_CAP_PROP_FRAME_HEIGHT)));
	int numFrames = (int)capture.get(CV_CAP_PROP_FRAME_COUNT);

	vector<double> sigema;
	vector<double> weight;
	for (int i = 0; i < 3; i++)
		weight.push_back(1. / 3);

	sigema.push_back(30);
	sigema.push_back(150);
	sigema.push_back(300);
	Mat frame, imgdst;
	Mat temp, temp2;
	int h1[3][256];
	int h2[3][256];
	DWORD t1, t2, t3;

	clock_t start, end;
	start = clock();
	for (int i = 1; i <= 20; i++)
	{
		printf("%d ", i);
		capture.read(frame);
		frame.copyTo(imgdst);
		t1 = GetTickCount();

		if (i % 100 == 1)
		{
			resize(frame, temp, Size(frame.size().width * 0.5, frame.size().height * 0.5));
			MultiScaleRetinexCRmat(temp, temp2, weight, sigema, 80, 80, 6, 2);
			generatehistogram(temp2, h1);
		}
		t2 = GetTickCount();
		generatehistogram(imgdst, h2);
		histogram_Eq(h2, h1, imgdst);
		//	histogram_Matching(imgdst, temp2);
		t3 = GetTickCount();

		printf("%d %d| ", t2 - t1, t3 - t2);



		out << imgdst;

		waitKey();

	}
	end = clock();
	cout << endl << "elapse:" << double(end - start) / CLOCKS_PER_SEC << endl;

	capture.release();
	out.release();
	system("pause");
	return 0;
}
