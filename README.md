# Socket-Group-Chat
多人聊天程序。

coding 环境：VS2015 WIN10

测试环境：VM虚拟机-->XP系统、2003server和主机WIN10

语言：C

功能：基于服务器转发消息的多人聊天，中途切换聊天对象只需要按下ESC然后输入一个存在的client name就可以了。

主要原理：client都连上server，然后server给client分配一个不同的ID，然后根据client的ID和需要chat的client ID转发消息。

