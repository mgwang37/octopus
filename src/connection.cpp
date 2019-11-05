#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <sys/epoll.h>
#include <errno.h>
#include <stdio.h>

#include "connection.h"
#include "message_log.h"
#include "dns_server.h"
#include "basic.h"

Connection::Method Connection::m_Method = Connection::eMethodNull;

map<string, string> Connection::m_UserList;

Profile *Connection::m_Profile;

void Connection::SetSocketAttr (int socket)
{
	struct linger pp;

	pp.l_onoff = 1;
	pp.l_linger = 0;

	setsockopt (socket, SOL_SOCKET, SO_LINGER, &pp, sizeof(pp));

	unsigned int timeout = STEP_TIME_OUT;

	setsockopt(socket, IPPROTO_TCP, TCP_USER_TIMEOUT, &timeout, sizeof(timeout));

	int keep_alive = 1, keep_idle = 10, keep_interval = 1, keep_count = 5;

	setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, &keep_alive, sizeof(keep_alive));
	setsockopt(socket, IPPROTO_TCP, TCP_KEEPIDLE, &keep_idle, sizeof(keep_idle));
	setsockopt(socket, IPPROTO_TCP, TCP_KEEPINTVL, &keep_interval, sizeof(keep_interval));
	setsockopt(socket, IPPROTO_TCP, TCP_KEEPCNT, &keep_count, sizeof(keep_count));
}


Connection::Connection ()
{
	m_Step = eStepNew;
}

Connection::~Connection ()
{
	if (m_ControlSd > 0)
	{
		close (m_ControlSd);
		m_ControlSd = -1;
	}

	if (m_ConnectSd > 0)
	{
		close (m_ConnectSd);
		m_ConnectSd = -1;
	}
}

void Connection::InitNewOne (int new_socket, long p_time, int epollfd)
{
	m_ControlSd = new_socket;
	m_ConnectSd = -1;;
	m_Step = Connection::eStepGetMethod;
	m_OutStart = 0;
	m_OutStop = 0;
	m_InStart = 0;
	m_InStop = 0;
	m_LastTime = p_time;
	m_Epollfd = epollfd;
}

void Connection::DoWork (int type, uint32_t mask, long p_time)
{
	switch (m_Step)
	{
	case eStepGetMethod:
		DoGetMethod (type, mask, p_time);
		break;
   case eStepGetAccess:
		DoGetAccess (type, mask, p_time);
		break;
   case eStepGetProtocol:
		DoGetProtocol (type, mask, p_time);
		break;
   case eStepTcpCreat:
		DoTcpCreat (type, mask, p_time);
		break;
   case eStepTcpConnecting:
		DoTcpConnecting (type, mask, p_time);
		break;
   case eStepTcpConnected:
		DoTcpConnected (type, mask, p_time);
		break;
   case eStepTcpClosing:
		DoTcpClosing (type, mask, p_time);
		break;
	default:
		LOGE ("Connection::DoWork switch (%d) type = %d !!\n", m_Step, type);
	}
}

void Connection::DoGetMethod (int type, uint32_t mask, long p_time)
{
	//超时处理
	if (type != 1)
	{
		if (p_time - m_LastTime > STEP_TIME_OUT)
		{
			m_Step = eStepClosed;
			close (m_ControlSd);
			m_ControlSd = -1;
			LOGE ("[DoGetMethod] time out !!\n");
		}
		return ;
	}

	/****/
	m_OutStop = recv (m_ControlSd, m_OutBuffer, BUFFER_LEN, MSG_DONTWAIT);
	if (m_OutStop <= 0)
	{
		m_Step = eStepClosed;
		close (m_ControlSd);
		m_ControlSd = -1;
		return ;
	}

	if (!((m_OutBuffer[0] == 5) && (m_OutBuffer[1] == m_OutStop-2)))
	{
		m_Step = eStepClosed;
		close (m_ControlSd);
		m_ControlSd = -1;
		return ;
	}

	int p_method = -1;
	for (int i= 2; i < m_OutStop; i++)
	{
		if (m_OutBuffer[i] == m_Method)
		{
			p_method = m_Method;
			break;
		}
	}

	if (p_method == -1)
	{
		m_OutBuffer[0] = 0x05;
		m_OutBuffer[1] = 0xFF;
		send (m_ControlSd, m_OutBuffer, 2, MSG_DONTWAIT|MSG_NOSIGNAL);
		m_Step = eStepTcpClosing;
		m_LastTime = p_time;
	}
	else
	{
		m_OutBuffer[0] = 0x05;
		m_OutBuffer[1] = m_Method;
		send (m_ControlSd, m_OutBuffer, 2, MSG_DONTWAIT|MSG_NOSIGNAL);

		switch (m_Method)
		{
		case 0x00:
			m_Step = eStepGetProtocol;
			break;
		case 0x01:
			m_Step = eStepGetAccess;
			break;
		case 0x02:
			m_Step = eStepGetAccess;
			break;
		}
		m_LastTime = p_time;
	}

}

bool Connection::LoadUserList (char *file)
{
	FILE  *fp;
	char  *line = NULL;
	int    read;
	size_t len = 0;

	fp = fopen (file, "r");
	if (NULL == fp)
	{
		return false;
	}

	while ((read = getline(&line, &len, fp)) != -1)
	{
		int  rett;
		char name[256];
		char password[256];

		rett = sscanf (line, "%s %s", name, password);
		if (rett != 2)
		{
			fclose (fp);
			return false;
		}

		m_UserList.insert(pair<string, string>(name, password));
	}

	fclose (fp);
	return true;
}

int Connection::AnalyzeProtocolRequests (char *p_mem, int length, int &cmd)
{
	if (p_mem[0] != 0x05)
	{
		return 1;
	}

	cmd = p_mem[1];

	if (p_mem[2] != 0x00)
	{
		return 1;
	}

	int p_len ;
	switch (p_mem[3])
	{
	case 0x01:
		m_ClientAddrOK = true;
		m_ClientAddr.sin6_family = AF_INET6;

		SetIPv6AddrWithIPv4 (&m_ClientAddr, &p_mem[4]);
		memcpy (&m_ClientAddr.sin6_port, &p_mem[8], 2);
		break;
	case 0x03:
		m_ClientAddrOK = false;
		p_len = p_mem[4];
		memcpy (m_Domainname, &p_mem[5], p_len);
		m_Domainname[p_len] = 0;
		memcpy (&m_ClientAddr.sin6_port, &p_mem[5 + p_len], 2);
		DnsServer::DoDNS (this);
		m_ClientAddr.sin6_family = AF_INET6;
		break;
	case 0x04:
		m_ClientAddrOK = true;
		m_ClientAddr.sin6_family = AF_INET6;
		SetIPv6AddrWithIPv6 (&m_ClientAddr, &p_mem[4]);
		memcpy (&m_ClientAddr.sin6_port, &p_mem[20], 2);
		break;
	default:
		return 2;
	}

	return 0;
}

void Connection::DoGetProtocol (int type, uint32_t mask, long p_time)
{
	//超时处理
	if (type != 1)
	{
		if (p_time - m_LastTime > STEP_TIME_OUT)
		{
			m_Step = eStepClosed;
			close (m_ControlSd);
			m_ControlSd = -1;
			LOGE ("[DoGetProtocol] time out !!\n");
		}
		return ;
	}

	/****/
	m_OutStop = recv (m_ControlSd, m_OutBuffer, BUFFER_LEN, MSG_DONTWAIT);
	if (m_OutStop <= 0)
	{
		m_Step = eStepClosed;
		close (m_ControlSd);
		m_ControlSd = -1;
		return ;
	}

	/****/
	int p_cmd = -1;
	int b_ret;

	b_ret = AnalyzeProtocolRequests (m_OutBuffer, m_OutStop, p_cmd);

	if (b_ret == 1)
	{
		m_Step = eStepClosed;
		close (m_ControlSd);
		m_ControlSd = -1;
		return ;
	}
	else if (b_ret == 2)
	{
		m_OutBuffer[1] = 0x08;
		send (m_ControlSd, m_OutBuffer, m_OutStop, MSG_DONTWAIT|MSG_NOSIGNAL);
		m_Step = eStepTcpClosing;
		m_LastTime = p_time;
		return ;
	}

	switch (p_cmd)
	{
	case 0x01:
		if (m_ClientAddrOK == true)
		{
			m_Step = eStepTcpCreat;
			m_LastTime = p_time;
			DoWork (type, mask, p_time);
		}
		else
		{
			m_Step = eStepTcpCreat;
			m_LastTime = p_time;
		}
		break;
	case 0x02:
		break;
	case 0x03:
		break;
	default:
		m_OutBuffer[1] = 0x07;
		send (m_ControlSd, m_OutBuffer, m_OutStop, MSG_DONTWAIT|MSG_NOSIGNAL);
		m_Step = eStepTcpClosing;
		m_LastTime = p_time;
		return ;
	}

}

void Connection::DoGetAccess (int type, uint32_t mask, long p_time)
{
	char user_name[256];
	char pass_word[256];
	int ULEN;
	int PLEN;

	m_OutStop = recv (m_ControlSd, m_OutBuffer, BUFFER_LEN, MSG_DONTWAIT);

	if (m_OutBuffer[0] != 1)
	{
		m_Step = eStepTcpClosing;
		return;
	}

	ULEN = m_OutBuffer[1];

	if (ULEN >= m_OutStop)
	{
		m_Step = eStepTcpClosing;
		return;
	}

	bzero (user_name, 256);
	memcpy (user_name, &m_OutBuffer[2], ULEN);

	PLEN = m_OutBuffer[2 + ULEN];

	if (m_OutStop != (ULEN + PLEN + 3))
	{
		m_Step = eStepTcpClosing;
		return;
	}

	bzero (pass_word, 256);
	memcpy (pass_word, &m_OutBuffer[3 + ULEN], PLEN);

	map<string, string>::iterator iter;

	iter = m_UserList.find (user_name);

	if (iter == m_UserList.end())
	{
		m_OutBuffer[0] = 1;
		m_OutBuffer[1] = 1;
		m_Step = eStepTcpClosing;
	}
	else
	{
		if (iter->second == pass_word)
		{
			m_OutBuffer[0] = 1;
			m_OutBuffer[1] = 0;
			m_Step = eStepGetProtocol;
		}
		else
		{
			m_OutBuffer[0] = 1;
			m_OutBuffer[1] = 1;
			m_Step = eStepTcpClosing;
		}
	}

	send (m_ControlSd, m_OutBuffer, 2, MSG_DONTWAIT|MSG_NOSIGNAL);
	m_OutStart = 0;
	m_OutStop = 0;
	m_LastTime = p_time;
}

void Connection::DoTcpCreat (int type, uint32_t mask, long p_time)
{
	if (false == m_ClientAddrOK)
	{
		if (p_time - m_LastTime > STEP_TIME_OUT*5)
		{
			m_OutBuffer[1] = 0x06;
			send (m_ControlSd, m_OutBuffer, m_OutStop, MSG_DONTWAIT|MSG_NOSIGNAL);
			m_Step = eStepTcpClosing;
			m_LastTime = p_time;
			LOGE ("[DoTcpCreat] time out !!\n");
		}
		return;
	}

	m_ConnectSd = socket (AF_INET6, SOCK_STREAM|SOCK_NONBLOCK, 0);
	if (m_ConnectSd == -1)
	{
		m_OutBuffer[1] = 0x01;
		send (m_ControlSd, m_OutBuffer, m_OutStop, MSG_DONTWAIT|MSG_NOSIGNAL);
		m_Step = eStepTcpClosing;
		m_LastTime = p_time;
		LOGE ("DoTcpCreat !!\n");
		return ;
	}

	SetSocketAttr (m_ConnectSd);

	struct  sockaddr_in6 *p_ip_addr = m_Profile->GetAddr ();

	if (p_ip_addr)
	{
		int ret;

		ret = bind (m_ConnectSd, (const struct sockaddr *)p_ip_addr, sizeof(sockaddr_in6));

		if (ret == -1)
		{
			close (m_ConnectSd);
			m_ConnectSd = -1;
			m_Step = eStepTcpClosing;
			LOGE ("TcpBind !!\n");
			return;
		}
		LOGI ("bind OK!\n");
	}

	connect (m_ConnectSd, (struct sockaddr *)&m_ClientAddr, sizeof(m_ClientAddr));
	m_Step = eStepTcpConnecting;
	m_LastTime = p_time;
}

void Connection::DoTcpConnecting (int type, uint32_t mask, long p_time)
{
	int       p_err;
	socklen_t p_len;

	p_len = sizeof (int);
	getsockopt (m_ConnectSd, SOL_SOCKET, SO_ERROR, &p_err, &p_len);
	if (p_err == EINPROGRESS)
	{
		if (p_time - m_LastTime > STEP_TIME_OUT*4)
		{
			m_Step = eStepTcpClosing; 
			m_LastTime = p_time;
			LOGE ("DoTcpConnecting:EINPROGRESS");
		}
		return;
	}
	else if (p_err == ENETUNREACH)
	{
		m_OutBuffer[1] = 0x03;
		send (m_ControlSd, m_OutBuffer, m_OutStop, MSG_DONTWAIT|MSG_NOSIGNAL);
		m_Step = eStepTcpClosing; 
		m_LastTime = p_time;
		LOGE ("DoTcpConnecting:ENETUNREACH");
		return;
	}
	else if (p_err == ECONNREFUSED)
	{
		m_OutBuffer[1] = 0x05;
		send (m_ControlSd, m_OutBuffer, m_OutStop, MSG_DONTWAIT|MSG_NOSIGNAL);
		m_Step = eStepTcpClosing; 
		m_LastTime = p_time;
		LOGE ("DoTcpConnecting:ECONNREFUSED");
		return;
	}

	struct epoll_event  ev;
	uint64_t p_64id = m_Index;

	ev.events = EPOLLRDHUP | EPOLLIN;
	ev.data.u64 = p_64id << 32 | 0x02;

	if (epoll_ctl(m_Epollfd, EPOLL_CTL_ADD, m_ConnectSd, &ev) == -1)
	{
		LOGE ("Connection::DoTcpConnecting  epoll_ctl !!\n");
	}

	m_OutBuffer[1] = 0;
	send (m_ControlSd, m_OutBuffer, m_OutStop, MSG_DONTWAIT|MSG_NOSIGNAL);

	m_Step = eStepTcpConnected;

	m_OutStart = 0;
	m_OutStop = 0;
}

void Connection::SetProfile (Profile *file)
{
	m_Profile = file;
}

void Connection::DoTcpConnected (int type, uint32_t mask, long p_time)
{
	if (m_OutStart == m_OutStop)
	{
		m_OutStart = 0;
		m_OutStop = 0;
	}

	if (m_InStart == m_InStop)
	{
		m_InStart = 0;
		m_InStop = 0;
	}

	if (m_ControlSd > 0 && BUFFER_LEN > m_OutStop)
	{
		int ret_sum;

		ret_sum = recv (m_ControlSd, &m_OutBuffer[m_OutStop], BUFFER_LEN - m_OutStop, MSG_DONTWAIT);
		if (ret_sum > 0)
		{
			m_OutStop += ret_sum;
		}
		else if (ret_sum == 0)
		{
			close (m_ControlSd);
			m_ControlSd = -1;
			m_Step = eStepTcpClosing;
			m_LastTime = p_time;
		}
	}

	if (m_ConnectSd > 0 && BUFFER_LEN > m_InStop)
	{
		int ret_sum;

		ret_sum = recv (m_ConnectSd, &m_InBuffer[m_InStop], BUFFER_LEN - m_InStop, MSG_DONTWAIT);
		if (ret_sum > 0)
		{
			m_InStop += ret_sum;
		}
		else if (ret_sum == 0)
		{
			close (m_ConnectSd);
			m_ConnectSd = -1;
			m_Step = eStepTcpClosing;
			m_LastTime = p_time;
		}
	}

	
	if (m_ControlSd > 0 && m_InStop > m_InStart)
	{
		int ret_sum;

		ret_sum = send (m_ControlSd, &m_InBuffer[m_InStart], m_InStop - m_InStart, MSG_DONTWAIT|MSG_NOSIGNAL);
		if (ret_sum > 0)
		{
			m_InStart += ret_sum;
		}
		else if (ret_sum < 0 && errno != EAGAIN)
		{
			close (m_ControlSd);
			m_ControlSd = -1;
			m_Step = eStepTcpClosing;
			m_LastTime = p_time;
		}
	}

	if (m_ConnectSd > 0 && m_OutStop > m_OutStart)
	{
		int ret_sum;

		ret_sum = send (m_ConnectSd, &m_OutBuffer[m_OutStart], m_OutStop - m_OutStart, MSG_DONTWAIT|MSG_NOSIGNAL);
		if (ret_sum > 0)
		{
			m_OutStart += ret_sum;
		}
		else if (ret_sum < 0 && errno != EAGAIN)
		{
			//perror ("send");
			close (m_ConnectSd);
			m_ConnectSd = -1;
			m_Step = eStepTcpClosing;
			m_LastTime = p_time;
		}
	}

	if (mask & EPOLLRDHUP)
	{
		if (type == 1)
		{
			close (m_ControlSd);
			m_ControlSd = -1;
			m_Step = eStepTcpClosing;
			m_LastTime = p_time;
		}

		if (type == 2)
		{
			close (m_ConnectSd);
			m_ConnectSd = -1;
			m_Step = eStepTcpClosing;
			m_LastTime = p_time;
		}
	}

}

void Connection::DoTcpClosing (int type, uint32_t mask, long p_time)
{

	if (p_time - m_LastTime > STEP_TIME_OUT*3)
	{
		if (m_ControlSd > 0)
		{
			close (m_ControlSd);
			m_ControlSd = -1;
		}

		if (m_ConnectSd > 0)
		{
			close (m_ConnectSd);
			m_ControlSd = -1;
		}
	}


	if (m_ControlSd > 0)
	{
		if (m_InStop - m_InStart > 0)
		{
			int ret_sum; 

			ret_sum = send (m_ControlSd, &m_InBuffer[m_InStart], m_InStop - m_InStart, MSG_DONTWAIT|MSG_NOSIGNAL);
			if (ret_sum > 0)
			{
				m_InStart += ret_sum;
			}
			else if (ret_sum < 0 && errno != EAGAIN)
			{
				close (m_ControlSd);
				m_ControlSd = -1;
			}
		}
		else if (m_InStop == m_InStart)
		{
			close (m_ControlSd);
			m_ControlSd = -1;
		}
	}

	if (m_ConnectSd > 0)
	{
		if (m_OutStop > m_OutStart)
		{
			int ret_sum;

			ret_sum = send (m_ConnectSd, &m_OutBuffer[m_OutStart], m_OutStop - m_OutStart, MSG_DONTWAIT|MSG_NOSIGNAL);
			if (ret_sum > 0)
			{
				m_OutStart += ret_sum;
			}
			else if (ret_sum < 0 && errno != EAGAIN)
			{
				close (m_ConnectSd);
				m_ConnectSd = -1;
			}
		}
		else if (m_OutStop == m_OutStart)
		{
			close (m_ConnectSd);
			m_ConnectSd = -1;
		}
	}

	if (m_ConnectSd == -1 && m_ControlSd == -1)
	{
		m_Step = eStepClosed;
	}

}

