#include "stdafx.h"
#include "Client.h"

pClient head = (pClient)malloc(sizeof(_Client)); //创建一个头结点

/*
* function  初始化链表
* return    无返回值
*/
void Init()
{
	head->next = NULL;
}

/*
* function  获取头节点
* return    返回头节点
*/
pClient GetHeadNode()
{
	return head;
}

/*
* function  初始化客户端信息
* param		客户端指针
* return    无返回值
*/

void InitClient(pClient pclient)
{
	memset(pclient->buf, 0, MAXSIZE);
	memset(pclient->userName, 0, sizeof(pclient->userName));
	memset(pclient->ChatName, 0, sizeof(pclient->ChatName));
	memset(pclient->IP, 0, sizeof(pclient->IP));
	pclient->sClient = INVALID_SOCKET;
	pclient->bStatus = TRUE;
}

/*
* function	添加一个客户端
* param     client表示一个客户端对象
* return    无返回值
*/
void AddClient(pClient client)
{
	client->next = head->next;  //比如：head->1->2,然后添加一个3进来后是
	head->next = client;        //3->1->2,head->3->1->2
}

/*
* function	删除一个客户端
* param     flag标识一个客户端对象
* return    返回true表示删除成功，false表示失败
*/
bool RemoveClient(UINT_PTR flag)
{
	//从头遍历，一个个比较
	pClient pCur = head->next;//pCur指向第一个结点
	while (pCur)
	{
		// head->1->2->3->4,要删除2，则直接让1->3
		if (pCur->flag == flag)
		{
			pCur->bStatus = false;       //下线
			memset(pCur->IP, 0, sizeof(pCur->IP));   //清理一下客户端信息
			memset(pCur->ChatName, 0, sizeof(pCur->ChatName));
			memset(pCur->buf, 0, MAXSIZE);
			pCur->Port = 0;
			pCur->flag = 0;
			pCur->sClient = INVALID_SOCKET;
			return true;
		}
		pCur = pCur->next;
	}
	return false;
}

/*
* function  查找指定客户端
* param     name是指定客户端的用户名
* return    返回socket表示查找成功，返回INVALID_SOCKET表示无此用户
*/
SOCKET FindClient(char* name)
{
	//从头遍历，一个个比较
	pClient pCur = head;
	while (pCur = pCur->next)
	{
		if (strcmp(pCur->userName, name) == 0)
			return pCur->sClient;
	}
	return INVALID_SOCKET;
}

/*
* function  根据SOCKET查找指定客户端
* param     client是指定客户端的套接字
* return    返回一个pClient表示查找成功，返回NULL表示无此用户
*/
pClient FindClient(SOCKET client)
{
	//从头遍历，一个个比较
	pClient pCur = head;
	while (pCur = pCur->next)
	{
		if (pCur->sClient == client)
			return pCur;
	}
	return NULL;
}

/*
* function  查找此用户是否存在（只要登陆过服务器就算存在）
* param	    根据用户名查找
* return	返回一个pClient表示查找成功，返回NULL表示无此用户
*/
pClient ClientExits(char* name)
{
	//从头遍历，一个个比较
	pClient pCur = head;
	while (pCur = pCur->next)
	{
		if (strcmp(pCur->userName, name) == 0)
			return pCur;
	}
	return NULL;
}

/*
* function  查找用户是否在线
* param		根据用户名查找
* return    返回TRUE表示在线，否则失败
*/
BOOL IsOnline(char* name)
{
	pClient pCur = head;
	while (pCur = pCur->next)
	{
		pClient pclient = ClientExits(name);
		if (pclient)
		{
			if (pclient->bStatus)
				return TRUE;
		}
	}
	return FALSE;
}

/*
* function  计算客户端连接数
* param     client表示一个客户端对象
* return    返回连接数
*/
int CountCon()
{
	int iCount = 0;
	pClient pCur = head;
	while (pCur = pCur->next)
	{
		if(pCur->bStatus)
			iCount++;
	}	
	return iCount;
}

/*
* function  清空链表
* return    无返回值
*/
void ClearClient()
{
	pClient pCur = head->next;
	pClient pPre = head;
	while (pCur)
	{
		//head->1->2->3->4,先删除1，head->2,然后free 1
		pClient p = pCur;
		pPre->next = p->next;
		free(p);
		pCur = pPre->next;
	}
}

/*
* function 检查连接状态并关闭一个连接
* return 返回值
*/
void CheckConnection(CListCtrl* pList)
{
	pClient pclient = GetHeadNode();
	while (pclient = pclient->next)
	{
		if (send(pclient->sClient, "", sizeof(""), 0) == SOCKET_ERROR)
		{
			if (pclient->sClient != INVALID_SOCKET)
			{
				
				LVFINDINFO lvFindInfo = { 0 };
				lvFindInfo.flags = LVFI_STRING;
				lvFindInfo.psz = pclient->userName;
				pList->SetItemText(pList->FindItem(&lvFindInfo), 3, "离线");
				char error[128] = { 0 };   //发送下线消息给发消息的人
				sprintf(error, "@%s.\n", pclient->userName);
				send(FindClient(pclient->ChatName), error, sizeof(error), 0);
				closesocket(pclient->sClient);   //这里简单的判断：若发送消息失败，则认为连接中断(其原因有多种)，关闭该套接字
				RemoveClient(pclient->flag);
				break;
			}
		}
	}
}


/*
* function  获取发送时间
* param		一个缓冲区
* return	无
*/
void GetTime(char* time)
{
	SYSTEMTIME st = { 0 };
	GetLocalTime(&st);
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
	sprintf(time, "%s:%s:%s", h, m, s);
}

/*
* function  指定发送给哪个客户端
* param     FromName，发信人
* param     ToName,   收信人
* param		data,	  发送的消息
*/
void SendData(char* FromName, char* ToName, char* data)
{
	SOCKET client = FindClient(ToName);   //查找是否有此用户
	char error[128] = { 0 };
	int ret = 0;
	if (client != INVALID_SOCKET)
	{
		char buf[MAXSIZE];
		char time[13] = { 0 };
		memset(buf, 0, sizeof(buf));
		GetTime(time);     //获取发送时间
		sprintf(buf, "%s  %s\r\n%s", FromName,time, data);   //添加发送消息的用户名
		ret = send(client, buf, strlen(buf)+1, 0);
	}
	else//发送错误消息给发消息的人
	{
		if(client == INVALID_SOCKET)
			sprintf(error, "The %s was downline.\n", ToName);
		else
			sprintf(error, "Send to %s message not allow empty, Please try again!\n", ToName);
		send(FindClient(FromName), error, sizeof(error), 0);
	}
	if (ret == SOCKET_ERROR)//发送下线消息给发消息的人
	{
		sprintf(error, "The %s was downline.\n", ToName);
		send(FindClient(FromName), error, sizeof(error), 0);
	}

}
