
// ServerDlg.h : 头文件
//

#pragma once
#include "afxcmn.h"


UINT ThreadSend(LPVOID lparam);
UINT ThreadRecv(LPVOID lparam);
UINT ThreadAccept(LPVOID lparam);
UINT ThreadManager(LPVOID lparam);

// CServerDlg 对话框
class CServerDlg : public CDialogEx
{
// 构造
public:
	CServerDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SERVER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


public:
	BOOL StartServer();//启动服务器
	void CloseServer();//关闭服务器
// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
	CListCtrl m_list;
};
