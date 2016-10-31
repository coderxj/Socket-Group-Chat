
// ClientDlg.h : 头文件
//

#pragma once
#include "afxwin.h"

#include <WinSock2.h>
#include <process.h>
#pragma comment(lib,"ws2_32.lib")



UINT ThreadRecv(LPVOID lparam);
UINT ThreadSend(LPVOID lparam);

// CClientDlg 对话框
class CClientDlg : public CDialogEx
{
// 构造
public:
	CClientDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CLIENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

public:
	BOOL  ConnectServer();   //连接服务器
	void CloseServer();      //关闭服务器
	CString GetTime();		 //获取发送和接收消息的时间
// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CString m_serverIP;		  //服务器IP
	CString m_userName;       //用户名
	CString m_chatName;       //聊天对象名
	CString m_showMsg;        //消息显示
	CString m_inputMsg;       //输入消息
	SOCKET m_clientSocket;    //客户端套接字 
	BOOL m_bChangeChat;       //是否切换聊天对象
	BOOL m_bConnection;       //是否断开与服务器的连接
	BOOL m_bSend;             //是否要发送消息
	
	afx_msg void OnBnClickedButtonConnectServer();
	afx_msg void OnBnClickedButtonChageChat();
	afx_msg BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg LRESULT OnUpdata(WPARAM wParam, LPARAM lParam);
};
