#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "assembly_line.h"
#include "message_log.h"

#define PTHREAD_STACK_SIZE (1024*1024*10)

#define  MAX_EVENTS   512

long AssemblyLine::m_HeartBeatTimes = 0;

static void *AssemblyLinePthreadMain (void *p_date)
{
	AssemblyLine *p_tmp = (AssemblyLine *)p_date;

	p_tmp->Run ();
	return NULL;
}

AssemblyLine::AssemblyLine ()
{
	m_ConnectionFreeList = NULL;
	m_ConnectionUseList = NULL;
	m_Connections = NULL;
}

AssemblyLine::~AssemblyLine ()
{
}

void AssemblyLine::DoRecovery ()
{
	Connection *p_current;
	Connection *p_prov;

	p_prov = NULL;
	p_current = m_ConnectionUseList;

	while (p_current)
	{

		if (p_current->m_Step == Connection::eStepClosed)
		{
			Connection *p_tmp;

			if (p_prov == NULL)
			{
				p_tmp = m_ConnectionUseList;
				m_ConnectionUseList = m_ConnectionUseList->m_next;
				FreeConnection (p_tmp);

				p_current = m_ConnectionUseList;
				p_prov = NULL;
			}
			else
			{
				p_tmp = p_current;

				p_current = p_current->m_next;
				p_prov->m_next = p_current;
				FreeConnection (p_tmp);
			}
			continue;
		}
		else
		{
			p_current->DoWork (0x03, 0x00, m_HeartBeatTimes);
		}

		p_prov = p_current;
		p_current = p_current->m_next;
	}
}

void AssemblyLine::NewConnection ()
{
	struct epoll_event ev;
	struct sockaddr_in6 client_addr;
	int new_socket;
	Connection *p_new_connect;
	uint64_t new_connect_id;
	socklen_t addrlen = sizeof(client_addr);

	new_socket = accept(m_ListenSd, (struct sockaddr *)&client_addr, &addrlen);
	
	Connection::SetSocketAttr (new_socket);

	p_new_connect = AllocConnection (new_connect_id);

	if (p_new_connect == NULL)
	{
		LOGE ("AllocConnection fail!!\n");
		close (new_socket);
		return;
	}

	p_new_connect->InitNewOne (new_socket, m_HeartBeatTimes, m_EpollFd);

	AddToUseList (p_new_connect);

	ev.events = EPOLLIN|EPOLLRDHUP;
	ev.data.u64 = (new_connect_id << 32) | 0x01;

	if (epoll_ctl(m_EpollFd, EPOLL_CTL_ADD, new_socket, &ev) == -1)
	{
		LOGE ("NewConnection epoll_ctl fail!!\n");
		exit(EXIT_FAILURE);
	}

}

void AssemblyLine::AddToUseList (Connection *p_tmp)
{
	p_tmp->m_next = m_ConnectionUseList;
	m_ConnectionUseList = p_tmp;
}

void AssemblyLine::DoWork ()
{
	struct epoll_event events[MAX_EVENTS];
	int nfds;

	nfds = epoll_wait(m_EpollFd, events, MAX_EVENTS, m_EpollWaitTime);
	if (nfds < 0)
	{
		return;
	}

	for (int i = 0; i < nfds; i++)
	{
		int p_fd, p_type;

		p_fd   = events[i].data.u64 >> 32;
		p_type = events[i].data.u64 & 0xffffffff;

		if (p_fd == m_ListenSd && p_type == 0)
		{
			NewConnection ();
		}
		else
		{
			m_Connections[p_fd].DoWork (p_type, events[i].events, m_HeartBeatTimes);
		}

	}

}

void AssemblyLine::Run ()
{
	m_WorkON = true;

	BindCPU ();
	InitConnectionPool ();

	m_ListenSd = CreatListenSd ();
	m_EpollFd  = CreatEpollFd ();

	while (m_WorkON)
	{
		DoWork ();
		if (m_HeartBeatTimes != m_HeartBeatTimesLast)
		{
			DoRecovery ();
			m_HeartBeatTimesLast = m_HeartBeatTimes;
		}
	}

	CloseEpollFd ();
	CloseListenSd ();
	FreeConnectionPool ();
}

void AssemblyLine::Start (int cpu_index)
{
	pthread_attr_t s5thread_attribute;

	pthread_attr_init(&s5thread_attribute);
	pthread_attr_setstacksize(&s5thread_attribute, PTHREAD_STACK_SIZE);
	pthread_attr_setscope(&s5thread_attribute, PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setdetachstate(&s5thread_attribute, PTHREAD_CREATE_DETACHED);

	m_CpuId = cpu_index;

	if(pthread_create(&m_ThreadID, &s5thread_attribute, &AssemblyLinePthreadMain, this) < 0)
	{
		LOGE ("pthread_create faile!!\n");
		exit (-1);
	}
}

int AssemblyLine::CreatListenSd ()
{
	int p_sd;

	p_sd = socket (AF_INET6, SOCK_STREAM, 0);
	if (p_sd < 0)
	{
		LOGE ("CreatListenSd error !!\n");
		exit (-1);
	}

	int opt = 1;
	if(setsockopt(p_sd, SOL_SOCKET, SO_REUSEPORT, (const void *)&opt, sizeof(opt)) == -1)
	{
		LOGE ("SO_REUSEPORT error !!\n");
		exit (-1);
	}

	if(setsockopt(p_sd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt)) == -1)
	{
		LOGE ("SO_REUSEADDR, error !!\n");
		exit (-1);
	}

	struct sockaddr_in6 addr;
	memset ((char*) &(addr),0, sizeof((addr)));
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons (m_ListenPort);
	addr.sin6_addr = in6addr_any;

	if (bind (p_sd, (struct sockaddr*)&addr, sizeof(addr)) != 0)
	{
		LOGE ("bind, error !!\n");
		exit (-1);
	}

	if (listen (p_sd, 1024) == -1)
	{
		LOGE ("listen, error !!\n");
		exit (-1);
	}

	return p_sd;
}

void AssemblyLine::CloseListenSd ()
{
	close (m_ListenSd);
	m_ListenSd = -1;
}

int AssemblyLine::CreatEpollFd ()
{
	struct   epoll_event ev;
	int      fd;
	uint64_t p_uin64;

	fd = epoll_create(MAX_EVENTS);

	if (fd == -1)
	{
		LOGE ("epoll_create error !!\n");
		exit (-1);
	}

	ev.events = EPOLLIN;
	p_uin64 = m_ListenSd;
	ev.data.u64 = p_uin64 << 32;

	if (epoll_ctl(fd, EPOLL_CTL_ADD, m_ListenSd, &ev) == -1)
	{
		LOGE("epoll_ctl: listen_sock\n");
		exit(EXIT_FAILURE);
	}

	return fd;
}

void AssemblyLine::CloseEpollFd ()
{
	close (m_EpollFd);
	m_EpollFd = -1;
}

void AssemblyLine::BindCPU ()
{
	int ret_val;

	cpu_set_t cpuset;

	CPU_ZERO (&cpuset);
	CPU_SET (m_CpuId, &cpuset);
	ret_val = pthread_setaffinity_np (m_ThreadID, sizeof(cpuset), &cpuset);

	if (ret_val == 0)
	{
		LOGE ("CPU[%d] BIND [OK] !\n", m_CpuId);
	}
}

void AssemblyLine::InitConnectionPool ()
{
	m_Connections = new Connection[m_ConnectSum]();

	for (int i =0; i < m_ConnectSum; i++)
	{
		FreeConnection (&m_Connections[i]);
	}
	m_client_sum = 0;
}

void AssemblyLine::FreeConnectionPool ()
{
	delete [] m_Connections;
	m_Connections = NULL;
}

Connection *AssemblyLine::AllocConnection (uint64_t &index)
{
	Connection *p_tmp;

	p_tmp = m_ConnectionFreeList;

	if (p_tmp == NULL)
	{
		return NULL;
	}

	m_ConnectionFreeList = m_ConnectionFreeList->m_next;

	index =  p_tmp - m_Connections;

	p_tmp->m_Index = index;
	p_tmp->m_Step = Connection::eStepNew;

	m_client_sum ++;
	LOGE ("++thread %d sum %d\n", m_CpuId, m_client_sum);

	return p_tmp;
}

void AssemblyLine::FreeConnection (Connection *p_tmp)
{
	p_tmp->m_Step = Connection::eStepFree;
	p_tmp->m_next = m_ConnectionFreeList;
	m_ConnectionFreeList = p_tmp;

	m_client_sum --;
	LOGE ("--thread %d sum %d\n", m_CpuId, m_client_sum);
}

void AssemblyLine::Stop ()
{
	m_WorkON = false;
}

void AssemblyLine::Init (int Listen_port, int connect_sum, int wait_time, int method)
{
	m_ListenPort = Listen_port;

	m_ConnectSum = connect_sum;

	m_EpollWaitTime = wait_time/1000;

	Connection::m_Method = (Connection::Method)method;
}

void AssemblyLine::HeartBeat (int cycle)
{
	m_HeartBeatTimes += cycle;
}

