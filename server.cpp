/*
 *本程序详解博客地址：http://blog.csdn.net/qq_18297675/article/details/52819975
 *本程序写于2016.10.14 12：00 -- 21：50

 *这个程序只是探讨多人聊天程序的雏形 -- 1V1聊天，但却不是简单的C-S对话，
 *而是C-S-C，两个client通过Sserver转发消息。
 
 *程序BUG如下：
 *当关闭一个连接的时候，CPU会突然疯涨，具体原因我测试了好多个，也没找出来，如果
 *有大神找出来了，希望能到博客下评论一下，感谢~

 *其他bug，因为没有时间专门测试，所以尚未知道。
*/


#include <WinSock2.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#pragma comment(lib,"ws2_32.lib")

#define SEND_OVER 1							 //已经转发消息
#define SEND_YET  0							 //还没转发消息

int g_iStatus = SEND_YET;
SOCKET g_ServerSocket = INVALID_SOCKET;		 //服务端套接字
SOCKADDR_IN g_ClientAddr = { 0 };			 //客户端地址
int g_iClientAddrLen = sizeof(g_ClientAddr);
bool g_bCheckConnect = false;                //检查连接情况
HANDLE g_hRecv1 = NULL;
HANDLE g_hRecv2 = NULL;
//客户端信息结构体
typedef struct _Client
{
	SOCKET sClient;      //客户端套接字
	char buf[128];		 //数据缓冲区
	char userName[16];   //客户端用户名
	char IP[20];		 //客户端IP
	UINT_PTR flag;       //标记客户端，用来区分不同的客户端
}Client;

Client g_Client[2] = { 0 };                  //创建一个客户端结构体

//发送数据线程
unsigned __stdcall ThreadSend(void* param)
{
	int ret = 0;
	int flag = *(int*)param;
	SOCKET client = INVALID_SOCKET;					//创建一个临时套接字来存放要转发的客户端套接字
	char temp[128] = { 0 };							//创建一个临时的数据缓冲区，用来存放接收到的数据
	memcpy(temp, g_Client[!flag].buf, sizeof(temp));
	sprintf(g_Client[flag].buf, "%s: %s", g_Client[!flag].userName, temp);//添加一个用户名头

	if (strlen(temp) != 0 && g_iStatus == SEND_YET) //如果数据不为空且还没转发则转发
		ret = send(g_Client[flag].sClient, g_Client[flag].buf, sizeof(g_Client[flag].buf), 0);
	if (ret == SOCKET_ERROR)
		return 1;
	g_iStatus = SEND_OVER;   //转发成功后设置状态为已转发
	return 0;
}

//接受数据
unsigned __stdcall ThreadRecv(void* param)
{
	SOCKET client = INVALID_SOCKET;
	int flag = 0; 
	if (*(int*)param == g_Client[0].flag)            //判断是哪个客户端发来的消息
	{
		client = g_Client[0].sClient;
		flag = 0;
	}	
	else if (*(int*)param == g_Client[1].flag)
	{
		client = g_Client[1].sClient;
		flag = 1;
	}
	char temp[128] = { 0 };  //临时数据缓冲区
	while (1)
	{
		memset(temp, 0, sizeof(temp));
		int ret = recv(client, temp, sizeof(temp), 0); //接收数据
		if (ret == SOCKET_ERROR)
			continue;
		g_iStatus = SEND_YET;								 //设置转发状态为未转发
		flag = client == g_Client[0].sClient ? 1 : 0;        //这个要设置，否则会出现自己给自己发消息的BUG
		memcpy(g_Client[!flag].buf, temp, sizeof(g_Client[!flag].buf));
		_beginthreadex(NULL, 0, ThreadSend, &flag, 0, NULL); //开启一个转发线程,flag标记着要转发给哪个客户端
		//这里也可能是导致CPU使用率上升的原因。
	}
		
	return 0;
}

//管理连接
unsigned __stdcall ThreadManager(void* param)
{
	while (1)
	{
		if (send(g_Client[0].sClient, "", sizeof(""), 0) == SOCKET_ERROR)
		{
			if (g_Client[0].sClient != 0)
			{
				CloseHandle(g_hRecv1); //这里关闭了线程句柄，但是测试结果断开连C/S接后CPU仍然疯涨
				CloseHandle(g_hRecv2);
				printf("Disconnect from IP: %s,UserName: %s\n", g_Client[0].IP, g_Client[0].userName);
				closesocket(g_Client[0].sClient);   //这里简单的判断：若发送消息失败，则认为连接中断(其原因有多种)，关闭该套接字
				g_Client[0] = { 0 };
			}
		}
		if (send(g_Client[1].sClient, "", sizeof(""), 0) == SOCKET_ERROR)
		{
			if (g_Client[1].sClient != 0)
			{
				CloseHandle(g_hRecv1);
				CloseHandle(g_hRecv2);
				printf("Disconnect from IP: %s,UserName: %s\n", g_Client[1].IP, g_Client[1].userName);
				closesocket(g_Client[1].sClient);
				g_Client[1] = { 0 };
			}
		}
		Sleep(2000); //2s检查一次
	}

	return 0;
}

//接受请求
unsigned __stdcall ThreadAccept(void* param)
{

	int i = 0;
	int temp1 = 0, temp2 = 0;
	_beginthreadex(NULL, 0, ThreadManager, NULL, 0, NULL);
	while (1)
	{
		while (i < 2)
		{
			if (g_Client[i].flag != 0)
			{
				++i;
				continue;
			}
			//如果有客户端申请连接就接受连接
			if ((g_Client[i].sClient = accept(g_ServerSocket, (SOCKADDR*)&g_ClientAddr, &g_iClientAddrLen)) == INVALID_SOCKET)
			{
				printf("accept failed with error code: %d\n", WSAGetLastError());
				closesocket(g_ServerSocket);
				WSACleanup();
				return -1;
			}
			recv(g_Client[i].sClient, g_Client[i].userName, sizeof(g_Client[i].userName), 0); //接收用户名
			printf("Successfuuly got a connection from IP:%s ,Port: %d,UerName: %s\n",
				inet_ntoa(g_ClientAddr.sin_addr), htons(g_ClientAddr.sin_port), g_Client[i].userName);
			memcpy(g_Client[i].IP, inet_ntoa(g_ClientAddr.sin_addr), sizeof(g_Client[i].IP)); //记录客户端IP
			g_Client[i].flag = g_Client[i].sClient; //不同的socke有不同UINT_PTR类型的数字来标识
			i++;
		}
		i = 0;
		
		if (g_Client[0].flag != 0 && g_Client[1].flag != 0)					 //当两个用户都连接上服务器后才进行消息转发
		{
			if (g_Client[0].flag != temp1)     //每次断开一个连接后再次连上会新开一个线程，导致cpu使用率上升,所以要关掉旧的
			{
				if (g_hRecv1)                  //这里关闭了线程句柄，但是测试结果断开连C/S接后CPU仍然疯涨
 					CloseHandle(g_hRecv1);
				g_hRecv1 = (HANDLE)_beginthreadex(NULL, 0, ThreadRecv, &g_Client[0].flag, 0, NULL); //开启2个接收消息的线程
			}	
			if (g_Client[1].flag != temp2)
			{
				if (g_hRecv2)
					CloseHandle(g_hRecv2);
				g_hRecv2 = (HANDLE)_beginthreadex(NULL, 0, ThreadRecv, &g_Client[1].flag, 0, NULL);
			}		
		}
		
		temp1 = g_Client[0].flag; //防止ThreadRecv线程多次开启
		temp2 = g_Client[1].flag;
			
		Sleep(2000);
	}

	return 0;
}

//启动服务器
int StartServer()
{
	//存放套接字信息的结构
	WSADATA wsaData = { 0 };
	SOCKADDR_IN ServerAddr = { 0 };				//服务端地址
	USHORT uPort = 18000;						//服务器监听端口

	//初始化套接字
	if (WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		printf("WSAStartup failed with error code: %d\n", WSAGetLastError());
		return -1;
	}
	//判断版本
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		printf("wVersion was not 2.2\n");
		return -1;
	}
	//创建套接字
	g_ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (g_ServerSocket == INVALID_SOCKET)
	{
		printf("socket failed with error code: %d\n", WSAGetLastError());
		return -1;
	}

	//设置服务器地址
	ServerAddr.sin_family = AF_INET;//连接方式
	ServerAddr.sin_port = htons(uPort);//服务器监听端口
	ServerAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);//任何客户端都能连接这个服务器

	//绑定服务器
	if (SOCKET_ERROR == bind(g_ServerSocket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)))
	{
		printf("bind failed with error code: %d\n", WSAGetLastError());
		closesocket(g_ServerSocket);
		return -1;
	}
	//设置监听客户端连接数
	if (SOCKET_ERROR == listen(g_ServerSocket, 20000))
	{
		printf("listen failed with error code: %d\n", WSAGetLastError());
		closesocket(g_ServerSocket);
		WSACleanup();
		return -1;
	}

	_beginthreadex(NULL, 0, ThreadAccept, NULL, 0, 0);
	for (int k = 0;k < 100;k++) //让主线程休眠，不让它关闭TCP连接.
		Sleep(10000000);
	
	//关闭套接字
	for (int j = 0;j < 2;j++)
	{
		if (g_Client[j].sClient != INVALID_SOCKET)
			closesocket(g_Client[j].sClient);
	}
	closesocket(g_ServerSocket);
	WSACleanup();
	return 0;
}

int main()
{
	StartServer(); //启动服务器
	
	return 0;
}