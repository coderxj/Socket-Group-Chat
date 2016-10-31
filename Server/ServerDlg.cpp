
// ServerDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "Server.h"
#include "ServerDlg.h"
#include "afxdialogex.h"
#include <WinSock2.h>
#include <process.h>
#include "Client.h"
#pragma comment(lib,"ws2_32.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


SOCKET g_ServerSocket = INVALID_SOCKET;		 //服务端套接字
SOCKADDR_IN g_ClientAddr = { 0 };			 //客户端地址
int g_iClientAddrLen = sizeof(g_ClientAddr);
bool g_bStartRecv = FALSE;
bool g_bStartServer = FALSE;				 //服务器是否启动

// CServerDlg 对话框

//发送数据线程
UINT ThreadSend(LPVOID lparam)
{
	pSend psend = (pSend)lparam;  //转换为Send类型
	SendData(psend->FromName, psend->ToName, psend->data); //发送数据
	return 0;
}

//接受数据
UINT ThreadRecv(LPVOID lparam)
{
	int ret = 0;
	while (1)
	{
		if (g_bStartRecv == false)
			return 1;
		pClient pclient = (pClient)lparam;
		if (!pclient)
			return 1;
		memset(pclient->buf, 0, MAXSIZE);
		ret = recv(pclient->sClient, pclient->buf, MAXSIZE, 0);
		if (ret == SOCKET_ERROR)
			return 1;
		if (strlen(pclient->buf) != 0)
		{
			if (pclient->buf[0] == '#' && pclient->buf[1] != '#') //#表示用户要指定另一个用户进行聊天
			{
				SOCKET socket = FindClient(&pclient->buf[1]);    //验证一下客户是否存在
				if (socket != INVALID_SOCKET)
				{
					pClient c = (pClient)malloc(sizeof(_Client));
					c = FindClient(socket);                        //只要改变ChatName,发送消息的时候就会自动发给指定的用户了
					memset(pclient->ChatName, 0, sizeof(pclient->ChatName));
					memcpy(pclient->ChatName, c->userName, sizeof(pclient->ChatName));
				}
				else
				{
					ret = send(pclient->sClient, "The user have not online or not exits.\r\n", 64, 0);
					if (ret == SOCKET_ERROR)
					{
						CString error;
						error.Format("send failed with error code: %d", WSAGetLastError());
						AfxMessageBox(error);
						return 1;
					}
				}

				continue;
			}

			pSend psend = (pSend)malloc(sizeof(_Send));
			memset(psend->FromName, 0, sizeof(psend->FromName));
			memset(psend->ToName, 0, sizeof(psend->ToName));
			memset(psend->data, 0, MAXSIZE);
			//把发送人的用户名和接收消息的用户和消息赋值给结构体，然后当作参数传进发送消息进程中
			memcpy(psend->FromName, pclient->userName, sizeof(psend->FromName));
			memcpy(psend->ToName, pclient->ChatName, sizeof(psend->ToName));
			memcpy(psend->data, pclient->buf, MAXSIZE);
			AfxBeginThread(ThreadSend, psend);
			Sleep(200);
		}
	}
	return 0;
}

//开启接收消息线程
void StartRecv()
{
	pClient pclient = GetHeadNode();
	g_bStartRecv = false;  //关闭所有接收线程
	Sleep(1000);
	g_bStartRecv = true;   //开启所有接收线程		
	while (pclient = pclient->next)
		AfxBeginThread(ThreadRecv, pclient);
}

UINT ThreadAccept(LPVOID lparam)
{
	AfxBeginThread(ThreadManager, lparam);
	Init(); //初始化一定不要再while里面做，否则head会一直为NULL！！！
	CServerDlg* pServer = (CServerDlg*)lparam;
	while (1)
	{
		int iCount = CountCon();  //获取在线人数
		//创建一个新的客户端对象
		pClient pclient = (pClient)malloc(sizeof(_Client));
		InitClient(pclient);  //初始化客户端

		//如果有客户端申请连接就接受连接
		if ((pclient->sClient = accept(g_ServerSocket, (SOCKADDR*)&g_ClientAddr, &g_iClientAddrLen)) == INVALID_SOCKET)
		{
			CString error;
			error.Format("accept failed with error code: %d", WSAGetLastError());
			closesocket(g_ServerSocket);
			WSACleanup();
			return -1;
		}
		recv(pclient->sClient, pclient->userName, sizeof(pclient->userName), 0); //接收用户名和指定聊天对象的用户名
		recv(pclient->sClient, pclient->ChatName, sizeof(pclient->ChatName), 0);

		memcpy(pclient->IP, inet_ntoa(g_ClientAddr.sin_addr), sizeof(pclient->IP)); //记录客户端IP
		pclient->flag = pclient->sClient; //不同的socke有不同UINT_PTR类型的数字来标识
		pclient->Port = htons(g_ClientAddr.sin_port);
		
		pClient pOldClient = ClientExits(pclient->userName);
		if (pOldClient)   //如果此用户之前已经登陆过，则激活他就行
		{
			pOldClient->bStatus = TRUE;
			pOldClient->sClient = pclient->sClient;
			pOldClient->Port = pclient->Port;
			pOldClient->flag = pclient->flag;
			memcpy(pOldClient->ChatName, pclient->ChatName, sizeof(pOldClient->ChatName));
			memcpy(pOldClient->IP, pclient->IP, sizeof(pOldClient->IP));
			LVFINDINFO lvFindInfo = { 0 };  //一列列修改
			lvFindInfo.flags = LVFI_STRING;
			lvFindInfo.psz = pOldClient->userName;
			int iItem = pServer->m_list.FindItem(&lvFindInfo);  //找到要修改的项
			pServer->m_list.SetItemText(iItem, 1, pOldClient->IP);
			CString s;
			s.Format("%d", pOldClient->Port);
			pServer->m_list.SetItemText(iItem, 2, s);
			pServer->m_list.SetItemText(iItem, 3, "在线");
			if (IsOnline(pOldClient->ChatName))
			{
				char name[16];
				sprintf(name, "$%s", pOldClient->userName);
				send(pOldClient->sClient, name, sizeof(name), 0);//发送在线状态给用户
				send(FindClient(pOldClient->ChatName), name, sizeof(name), 0);  
			}
		}
		else
		{
			AddClient(pclient);  //把新的客户端加入链表中
			pServer->m_list.InsertItem(iCount, pclient->userName);
			pServer->m_list.SetItemText(iCount, 1, pclient->IP);
			CString s;
			s.Format("%d", pclient->Port);
			pServer->m_list.SetItemText(iCount, 2, s);
			pServer->m_list.SetItemText(iCount, 3, "在线");
			if (IsOnline(pclient->ChatName))
			{
				char name[16];
				sprintf(name, "$%s", pclient->userName);
				SOCKET chatSocket = FindClient(pclient->ChatName);
				send(pclient->sClient, name, sizeof(name), 0);//发送在线状态给用户
				if(chatSocket != INVALID_SOCKET)
					send(chatSocket, name, sizeof(name), 0);
			}
		}			

		if (CountCon() >= 2)					 //当至少两个用户都连接上服务器后才进行消息转发                                                          
			StartRecv();

		Sleep(2000);  //其实这里没有必要睡眠2s，因为accept会阻塞
	}
	return 0;
}

UINT ThreadManager(LPVOID lparam)
{
	CServerDlg* pServer = (CServerDlg*)lparam;
	while (1)
	{
		CheckConnection(&pServer->m_list);  //检查连接状况
		pServer->SetDlgItemInt(IDC_STATIC_ONLINE_NUMS, CountCon());
		Sleep(2000);		//检测时间间隔越长 发送消息碰撞的概率越小，发送成功率越高
	}

	return 0;
}


CServerDlg::CServerDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_SERVER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST, m_list);
}

BEGIN_MESSAGE_MAP(CServerDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CServerDlg::OnBnClickedButton1)
END_MESSAGE_MAP()


// CServerDlg 消息处理程序

BOOL CServerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	//设置list表头
	m_list.InsertColumn(0, "用户名", 0, 120);
	m_list.InsertColumn(1, "IP", 0, 120);
	m_list.InsertColumn(2, "端口", 0, 80);
	m_list.InsertColumn(3, "状态", 0, 80);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CServerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CServerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

//启动服务器
BOOL CServerDlg::StartServer()
{
	//存放套接字信息的结构
	WSADATA wsaData = { 0 };
	SOCKADDR_IN ServerAddr = { 0 };				//服务端地址
	USHORT uPort = 18000;						//服务器监听端口
	CString error;
	//初始化套接字
	if (WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		error.Format("WSAStartup failed with error code: %d", WSAGetLastError());
		AfxMessageBox(error);
		return FALSE;
	}
	//判断版本
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		AfxMessageBox("wVersion was not 2.2");
		return FALSE;
	}
	//创建套接字
	g_ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (g_ServerSocket == INVALID_SOCKET)
	{
		error.Format("socket failed with error code: %d", WSAGetLastError());
		AfxMessageBox(error);
		return FALSE;
	}

	//设置服务器地址
	ServerAddr.sin_family = AF_INET;//连接方式
	ServerAddr.sin_port = htons(uPort);//服务器监听端口
	ServerAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);//任何客户端都能连接这个服务器

														//绑定服务器
	if (SOCKET_ERROR == bind(g_ServerSocket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)))
	{
		error.Format("bind failed with error code: %d", WSAGetLastError());
		AfxMessageBox(error);
		closesocket(g_ServerSocket);
		return FALSE;
	}
	//设置监听客户端连接数
	if (SOCKET_ERROR == listen(g_ServerSocket, 20000))
	{
		error.Format("listen failed with error code: %d", WSAGetLastError());
		AfxMessageBox(error);
		closesocket(g_ServerSocket);
		WSACleanup();
		return FALSE;
	}

	AfxBeginThread(ThreadAccept, this);

	return TRUE;
}

//关闭服务器
void CServerDlg::CloseServer()
{
	//关闭套接字
	ClearClient();
	closesocket(g_ServerSocket);
	WSACleanup();
	m_list.DeleteAllItems();
	g_bStartRecv = false;
}

//启动服务器
void CServerDlg::OnBnClickedButton1()
{
	if (g_bStartServer == FALSE)
	{
		if (StartServer())
		{
			SetDlgItemText(IDC_BUTTON1, "关闭");
			SetDlgItemText(IDC_STATIC_SERVER_STATUS, "开启");
			g_bStartServer = TRUE;
		}
	}
	else
	{
		CloseServer();
		SetDlgItemText(IDC_BUTTON1, "开启");
		SetDlgItemText(IDC_STATIC_SERVER_STATUS, "关闭");
		g_bStartServer = FALSE;
	}

}
