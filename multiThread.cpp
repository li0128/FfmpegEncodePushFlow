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

//定义线程函数传入参数的结构体
typedef struct __THREAD_DATA
{
	int nMaxNum;
	char strThreadName[NAME_LINE];

	__THREAD_DATA() : nMaxNum(0)
	{
		memset(strThreadName, 0, NAME_LINE * sizeof(char));
	}
}THREAD_DATA;



//线程函数
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
	//初始化线程数据
	THREAD_DATA threadData1, threadData2, threadData3;
	threadData1.nMaxNum = 5;
	strcpy(threadData1.strThreadName, "线程1");
	threadData2.nMaxNum = 10;
	strcpy(threadData2.strThreadName, "线程2");
	threadData3.nMaxNum = 10;
	strcpy(threadData3.strThreadName, "线程3");

	//创建第一个子线程
	HANDLE hThread1 = CreateThread(NULL, 0, producer, &threadData1, 0, NULL);
	//创建第二个子线程
	HANDLE hThread2 = CreateThread(NULL, 0, consumer, &threadData2, 0, NULL);
	//创建第三个子线程
	//HANDLE hThread3 = CreateThread(NULL, 0, ThreadProc, &threadData3, 0, NULL);
	//关闭线程
	CloseHandle(hThread1);
	CloseHandle(hThread2);
	//CloseHandle(hThread3);

	//主线程的执行路径
	//for (int i = 0; i < 5; ++i)
	//{
	//	cout << "主线程 === " << i << endl;
	//	Sleep(100);
	//}

	system("pause");
	return 0;
}