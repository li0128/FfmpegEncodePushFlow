//#include "stdafx.h"
#include <windows.h>
#include <iostream>

#include<queue>

using namespace std;

#define BUFFER_SIZE 20

//typedef int semaphore;

//semaphore fillCount = 0; // items produced
//semaphore emptyCount = BUFFER_SIZE; // remaining space

queue<int> buffer1;

int i = 1;
int produceItem() {
	cout << "ProduceItem:" << i << endl;
	return i++;
}

void consumeItem(int item) {
	cout << "ConsumeItem:" << item << endl;
}


DWORD WINAPI producer(LPVOID lpParameter)
{
	int item;

	while (true)
	{
		item = produceItem();
		if (BUFFER_SIZE - buffer1.size() > 0) {
			//emptyCount--;
			buffer1.push(item);
			//fillCount++;
		}
		Sleep(1);
	}
	return 0L;
}

DWORD WINAPI consumer(LPVOID lpParameter)
{
	int item;

	while (true)
	{
		if (buffer1.size() > 0) {
			//fillCount--;
			item = buffer1.front();
			buffer1.pop();
			//emptyCount++;
			consumeItem(item);
		}
		Sleep(1);
	}
	return 0L;
}

#define NAME_LINE   40

//�����̺߳�����������Ľṹ��
typedef struct __THREAD_DATA
{
	int nMaxNum;
	char strThreadName[NAME_LINE];

	__THREAD_DATA() : nMaxNum(0)
	{
		memset(strThreadName, 0, NAME_LINE * sizeof(char));
	}
}THREAD_DATA;



//�̺߳���
DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
	THREAD_DATA* pThreadData = (THREAD_DATA*)lpParameter;

	for (int i = 0; i < pThreadData->nMaxNum; ++i)
	{
		cout << pThreadData->strThreadName << " --- " << i << endl;
		Sleep(100);
	}
	return 0L;
}

int main()
{
	//��ʼ���߳�����
	THREAD_DATA threadData1, threadData2, threadData3;
	threadData1.nMaxNum = 5;
	strcpy(threadData1.strThreadName, "�߳�1");
	threadData2.nMaxNum = 10;
	strcpy(threadData2.strThreadName, "�߳�2");
	threadData3.nMaxNum = 10;
	strcpy(threadData3.strThreadName, "�߳�3");

	//������һ�����߳�
	HANDLE hThread1 = CreateThread(NULL, 0, producer, &threadData1, 0, NULL);
	//�����ڶ������߳�
	HANDLE hThread2 = CreateThread(NULL, 0, consumer, &threadData2, 0, NULL);
	//�������������߳�
	//HANDLE hThread3 = CreateThread(NULL, 0, ThreadProc, &threadData3, 0, NULL);
	//�ر��߳�
	CloseHandle(hThread1);
	CloseHandle(hThread2);
	//CloseHandle(hThread3);

	//���̵߳�ִ��·��
	//for (int i = 0; i < 5; ++i)
	//{
	//	cout << "���߳� === " << i << endl;
	//	Sleep(100);
	//}

	system("pause");
	return 0;
}