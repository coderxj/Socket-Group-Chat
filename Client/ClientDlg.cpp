
// ClientDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "Client.h"
#include "ClientDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define WM_UPDATA (WM_USER+0x10)  //自定义消息
#define MAXSIZE 1024	          //缓冲区大小

// CClientDlg 对话框

UINT ThreadRecv(LPVOID lparam)
{
	char buf[MAXSIZE] = { 0 };
	CClientDlg* pClient = (CClientDlg*)lparam;
	while (1)
	{
		if (pClient->m_bConnection == FALSE)  //关闭服务器就退出线程函数
			return 1;
		memset(buf, 0, MAXSIZE);
		int ret = recv(pClient->m_clientSocket, buf, MAXSIZE, 0);
		if (ret == SOCKET_ERROR)
		{
			Sleep(500);
			continue;
		}
		if (strlen(buf) != 0)
		{
			
			if (buf[0] == '@')   //这是好友下线标志
			{
				char name[16] = { 0 };
				sprintf(name, "%s", &buf[1]);
				pClient->SetDlgItemText(IDC_STATIC_F_STATIC, "离线");
			}
			else if (buf[0] == '$')    //这是好友上线标志
			{
				char name[16] = { 0 };
				sprintf(name, "%s", &buf[1]);
				pClient->SetDlgItemText(IDC_STATIC_F_STATIC, "在线");
			}
			else
			{
				char b[MAXSIZE] = { 0 };
				sprintf(b, "\n\r%s\n\r", buf);
				pClient->m_showMsg.Append(b);
				pClient->SendMessage(WM_UPDATA, 0); //UpdateData(FALSE);
			}
		}
		else
			Sleep(100);
	}
	return 0;
}

UINT ThreadSend(LPVOID lparam)
{
	int ret = 0;
	CClientDlg* pClient = (CClientDlg*)lparam;
	while (1)
	{
		if (pClient->m_bConnection == FALSE)  //关闭服务器就退出线程函数
			return 1;
		if (pClient->m_bChangeChat)      //发送改变聊天对象消息给服务器
		{
			pClient->SendMessage(WM_UPDATA, 1);//UpdateData(TRUE);
			char b[17] = { 0 };
			sprintf(b, "#%s", pClient->m_chatName);
			ret = send(pClient->m_clientSocket, b, strlen(b) + 1, 0);
			if (ret == SOCKET_ERROR)
			{
				CString error;
				error.Format("send failed with error code: %d", WSAGetLastError());
				AfxMessageBox(error);
				return 1;
			}
			pClient->m_bChangeChat = FALSE;   
			continue;
		}
		
		if (pClient->m_bSend == TRUE)  //发送消息
		{
			pClient->SendMessage(WM_UPDATA, 1);//UpdateData(TRUE);
			pClient->m_showMsg.Append(pClient->m_userName + "  " + pClient->GetTime() + "\r\n" + pClient->m_inputMsg + "\r\n");
			pClient->SendMessage(WM_UPDATA, 0);//UpdateData(FALSE);
			ret = send(pClient->m_clientSocket, pClient->m_inputMsg, strlen(pClient->m_inputMsg) + 1, 0); //这里不能用sizeof,当发送中文的时候计算字节大小会出问题
			if (ret == SOCKET_ERROR)
			{
				CString error;
				error.Format("send failed with error code: %d", WSAGetLastError());
				AfxMessageBox(error);
				return 1;
			}
			pClient->m_inputMsg.Empty();  //发送消息后要清空编辑框
			pClient->SendMessage(WM_UPDATA, 0);//UpdateData(FALSE);
			pClient->m_bSend = FALSE;
		}
		Sleep(100);  //这个真的很有必要，如果不休眠的话 1秒钟就会循环很多很多次，导致CPU使用率暴涨~！
	}
	return 0;
}


CClientDlg::CClientDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_CLIENT_DIALOG, pParent)
	, m_serverIP(_T("127.0.0.1"))
	, m_userName(_T(""))
	, m_chatName(_T(""))
	, m_inputMsg(_T(""))
	, m_showMsg(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_clientSocket = INVALID_SOCKET;
	m_bChangeChat = FALSE;
	m_bConnection = FALSE;
	m_bSend = FALSE;
}

void CClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_IP, m_serverIP);
	DDX_Text(pDX, IDC_EDIT_USERNAEM, m_userName);
	DDX_Text(pDX, IDC_EDIT_ChatName, m_chatName);
	DDX_Text(pDX, IDC_EDIT_SHOW_MSG, m_showMsg);
	DDX_Text(pDX, IDC_EDIT_INPUT_MSG, m_inputMsg);
}

BEGIN_MESSAGE_MAP(CClientDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_CONNECT_SERVER, &CClientDlg::OnBnClickedButtonConnectServer)
	ON_BN_CLICKED(IDC_BUTTON_CHAGE_CHAT, &CClientDlg::OnBnClickedButtonChageChat)
	ON_MESSAGE(WM_UPDATA,&CClientDlg::OnUpdata)
END_MESSAGE_MAP()


// CClientDlg 消息处理程序

BOOL CClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CClientDlg::OnPaint()
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
HCURSOR CClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

//连接服务器
BOOL  CClientDlg::ConnectServer()
{
	WSADATA wsaData = { 0 };//存放套接字信息
	SOCKADDR_IN ServerAddr = { 0 };//服务端地址
	USHORT uPort = 18000;//服务端端口
	CString error;
	//初始化套接字
	if (WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		error.Format("WSAStartup failed with error code: %d", WSAGetLastError());
		AfxMessageBox(error);
		return FALSE;
	}
	//判断套接字版本
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		AfxMessageBox("wVersion was not 2.2");
		return FALSE;
	}
	//创建套接字
	m_clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_clientSocket == INVALID_SOCKET)
	{
		error.Format("socket failed with error code: %d", WSAGetLastError());
		AfxMessageBox(error);
		return FALSE;
	}
	UpdateData(TRUE);
	//设置服务器地址
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(uPort);						//服务器端口
	ServerAddr.sin_addr.S_un.S_addr = inet_addr(m_serverIP);//服务器地址

	SetDlgItemText(IDC_STATIC_S_STATIC, "connecting......");//显示连接状态
	//连接服务器
	if (SOCKET_ERROR == connect(m_clientSocket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)))
	{
		error.Format("connect failed with error code: %d", WSAGetLastError());
		AfxMessageBox(error);
		closesocket(m_clientSocket);
		WSACleanup();
		return FALSE;
	}

	SetDlgItemText(IDC_STATIC_S_STATIC, "successfully");//显示连接状态
	int ret = 0;
	ret = send(m_clientSocket, m_userName, strlen(m_userName) + 1, 0);
	if (ret == SOCKET_ERROR)
	{
		error.Format("send failed with error code: %d", WSAGetLastError());
		AfxMessageBox(error);
		closesocket(m_clientSocket);
		WSACleanup();
		return FALSE;
	}
	ret = send(m_clientSocket, m_chatName, strlen(m_chatName) + 1, 0);
	if (ret == SOCKET_ERROR)
	{
		error.Format("send failed with error code: %d", WSAGetLastError());
		AfxMessageBox(error);
		closesocket(m_clientSocket);
		WSACleanup();
		return FALSE;
	}

	return TRUE;
}

//关闭服务器
void CClientDlg::CloseServer()
{
	closesocket(m_clientSocket);
	WSACleanup();
}

//连接服务器
void CClientDlg::OnBnClickedButtonConnectServer()
{
	if (m_bConnection == FALSE)
	{
		if (ConnectServer())  //连接服务器
		{
			SetDlgItemText(IDC_BUTTON_CONNECT_SERVER, "Disconnect");
			m_bConnection = TRUE;             //这里要注意，必须m_bConnection为TRUE才能开启线程，不然一开启就会关闭了
			AfxBeginThread(ThreadRecv, this); //启动接收和发送消息线程
			AfxBeginThread(ThreadSend, this);
		}
		else
			SetDlgItemText(IDC_STATIC_S_STATIC, "Connect failed");
		
	}
	else
	{
		CloseServer();   //关闭服务器
		SetDlgItemText(IDC_BUTTON_CONNECT_SERVER, "Connect Server");
		SetDlgItemText(IDC_STATIC_S_STATIC, "");
		m_bConnection = FALSE;
	}
		
}

//改变聊天对象
void CClientDlg::OnBnClickedButtonChageChat()
{
	if (m_bChangeChat == FALSE)
		m_bChangeChat = TRUE;
}

//消息预处理，拦截回车消息
BOOL CClientDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_RETURN)
		{
			if(pMsg->hwnd == GetDlgItem(IDC_EDIT_INPUT_MSG)->GetSafeHwnd())  //判断是不是输入框的回车消息
				m_bSend = TRUE;//如果按回车就可以发送消息了
			if (pMsg->hwnd == GetDlgItem(IDC_EDIT_ChatName)->GetSafeHwnd())  //当改变了ChatName回车就可以换了
			{
				OnBnClickedButtonChageChat();
				::SetFocus(GetDlgItem(IDC_EDIT_INPUT_MSG)->GetSafeHwnd());   //焦点定位到输入框
			}
				
			return 0; //这个一点要加上，不然登陆框就会闪退
		}
	}

	return CDialog::PreTranslateMessage(pMsg);
}

//自定义消息
LRESULT CClientDlg::OnUpdata(WPARAM wParam, LPARAM lParam)
{
	if (wParam == 0)
	{
		UpdateData(FALSE);
		CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_SHOW_MSG);
		pEdit->SendMessage(WM_VSCROLL, SB_BOTTOM);			//发送消息给控件  让它滑倒底部
	}
	else
		UpdateData(TRUE);
	return 0;
}

//获取发送和接收消息的时间
CString CClientDlg::GetTime()
{
	SYSTEMTIME st;
	GetLocalTime(&st);   //获取本地时间
	char h[3] = { 0 };
	char m[3] = { 0 };
	char s[3] = { 0 };
	if (st.wHour < 10)
		sprintf(h, "0%d", st.wHour);
	else
		sprintf(h, "%d", st.wHour);
	if (st.wMinute < 10)
		sprintf(m, "0%d", st.wMinute);
	else
		sprintf(m, "%d", st.wMinute);
	if (st.wSecond < 10)
		sprintf(s, "0%d", st.wSecond);
	else
		sprintf(s, "%d", st.wSecond);
	CString str;
	str.Format("%s:%s:%s",h,m,s);
	return str;
}
