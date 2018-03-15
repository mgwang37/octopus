#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <stdio.h>

#include <arpa/inet.h>

#include "profile.h"
#include "message_log.h"

void Profile::PrintfHelpInfo ()
{
	printf ("--listen.port               listen port\n");
	printf ("--thread.sum                thread sum\n");
	printf ("--log.dir                   log output dir\n");
	printf ("--thread.connections.sum    the max connections of per thread\n");
	printf ("--heartbeat.cycle           heartbeat cycle microseconds (must small 1000000!)\n");
	printf ("--access.method             access method : 0 (anonymous), 2(USERNAME/PASSWORD)\n");
	printf ("--userlist                  userlist file \n");
	printf ("--addr.list                 NIC address list, use like this --addr.list 192.168.1.1,192.168.1.2 \n");
	printf ("--help or -v                this message\n");
}

Profile::Profile (int argc, char *argv[]):m_ListenPort(1024),m_ThreadSum(1),m_LogDir(NULL),m_ConnectionPerThread (1000),
														m_HeartBeatCycle(100000),m_AccessMethod (0)
{
	int c;
	int option_index = 0;

	m_AddrListSum = 0;
	bzero (m_AddrListStr, ADDR_LIST_STR_MAX);

	struct option long_options[] =
	{
		{"listen.port", required_argument, 0,  0 },
		{"thread.sum", required_argument, 0,  0 },
		{"log.dir", required_argument, 0,  0 },
		{"thread.connections.sum", required_argument, 0,  0 },
		{"heartbeat.cycle", required_argument, 0,  0 },
		{"access.method", required_argument, 0,  0 },
		{"userlist", required_argument, 0,  0 },
		{"help", no_argument, 0,  0 },
		{"addr.list", required_argument, 0,  0 },
		{0 , 0, 0,  0}
	};

	while (1)
	{
		c = getopt_long(argc, argv, "h", long_options, &option_index);
		if (c == -1)
		{
			break;
		}

		if (c == 'h')
		{
			PrintfHelpInfo ();
			exit (0);
		}
		else if (c == 0)
		{
			switch (option_index)
			{
			case 0:
				m_ListenPort = atoi (optarg);
				break;
			case 1:
				m_ThreadSum = atoi (optarg);
				break;
			case 2:
				m_LogDir = strdup (optarg);
				break;
			case 3:
				m_ConnectionPerThread = atoi (optarg);
				break;
			case 4:
				m_HeartBeatCycle = atoi (optarg);
				break;
			case 5:
				m_AccessMethod = atoi (optarg);
				break;
			case 6:
				m_UserListFile = strdup (optarg);
				break;
			case 7:
				PrintfHelpInfo ();
				exit (0);
			case 8:
				GetAddrList (optarg);
				break;
			}
		}
	}

}

struct sockaddr_in6 *Profile::GetAddr()
{
	if (m_AddrListSum == 0)
	{
		return NULL;
	}

	if (m_AddrListIndex >= m_AddrListSum)
	{
		m_AddrListIndex = 0;
	}

	LOGI ("m_AddrListIndex = %d\n", m_AddrListIndex);

	return &m_addr_list[m_AddrListIndex++];
}

void  Profile::GetAddrList (char *optarg)
{
	char p_tmp[256];
	char *p_str;
	int  p_tmp_index = 0;

	m_AddrListStr[0] = 0;

	strcat (m_AddrListStr, optarg);

	p_str = &m_AddrListStr[0];

	if (*p_str == 0)
	{
		return;
	}

	while (1)
	{
		if (*p_str == '\0')
		{
			p_tmp[p_tmp_index] = 0;
			p_tmp_index = 0;

			m_addr_list[m_AddrListSum].sin6_family = AF_INET6;
			m_addr_list[m_AddrListSum].sin6_port = 0;
			inet_pton (AF_INET6, p_tmp, m_addr_list[m_AddrListSum].sin6_addr.s6_addr);
			m_AddrListSum ++;
			break;
		}
		else if (*p_str == ',')
		{
			p_tmp[p_tmp_index] = 0;
			p_tmp_index = 0;

			m_addr_list[m_AddrListSum].sin6_family = AF_INET6;
			m_addr_list[m_AddrListSum].sin6_port = 0;
			inet_pton (AF_INET6, p_tmp, m_addr_list[m_AddrListSum].sin6_addr.s6_addr);
			m_AddrListSum ++;
		}
		else
		{
			p_tmp[p_tmp_index] = *p_str;
		}

		p_str ++;
	}
}

uint32_t Profile::GetHeartBeatCycle ()
{
	return m_HeartBeatCycle;
}

int Profile::GetConnectionSum ()
{
	return m_ConnectionPerThread;
}

void Profile::ShowInfor ()
{
	LOGE ("=========ProfileInfor=============\n");
	LOGE (" m_LogDir = %s\n", m_LogDir);
	LOGE (" m_ListenPort = %d\n", m_ListenPort);
	LOGE (" m_ThreadSum = %d\n", m_ThreadSum);
	LOGE (" m_ConnectionPerThread = %d\n", m_ConnectionPerThread);
	LOGE (" m_HeartBeatCycle = %d\n", m_HeartBeatCycle);
	LOGE (" m_AccessMethod = %d\n", m_AccessMethod);
	LOGE (" m_UserListFile = %s\n", m_UserListFile);
	LOGE (" m_AddrListStr = %s\n", m_AddrListStr);
	LOGE ("==================================\n");
}

Profile::~Profile ()
{
	free (m_LogDir);
	m_LogDir = NULL;
	free (m_UserListFile);
	m_UserListFile = NULL;
}

char *Profile::GetLogDir ()
{
	return m_LogDir;
}

char *Profile::GetUserListFile ()
{
	return m_UserListFile;
}

int Profile::GetListenPort ()
{
	return m_ListenPort;
}

int Profile::GetThreadSum ()
{
	return m_ThreadSum;
}

int Profile::GetAccessMethod ()
{
	return m_AccessMethod;
}

