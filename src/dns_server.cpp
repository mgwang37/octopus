#include <strings.h>
#include <string.h>

#include <stdlib.h>
#include <pthread.h>

#include <sys/signalfd.h>
#include <signal.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "dns_server.h"
#include "message_log.h"
#include "basic.h"

#define PTHREAD_STACK_SIZE  (1024*1024*80)

int DnsServer::m_SigFd = -1;
pthread_t DnsServer::m_ThreadID;
bool DnsServer::m_WorkON = false;

void *dns_thread_main (void *p_date)
{
	DnsServer *p_server = (DnsServer*)p_date;

	p_server->DoWork ();
	return NULL;
}

DnsServer::DnsServer ()
{
}

DnsServer::~DnsServer ()
{
}

bool DnsServer::StartServer ()
{
	if (m_WorkON)
	{
		return false;
	}
	m_WorkON = true;

	pthread_attr_t s5thread_attribute;

	pthread_attr_init(&s5thread_attribute);
	pthread_attr_setstacksize(&s5thread_attribute, PTHREAD_STACK_SIZE);
	pthread_attr_setscope(&s5thread_attribute, PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setdetachstate(&s5thread_attribute, PTHREAD_CREATE_DETACHED);

	if(pthread_create(&m_ThreadID, &s5thread_attribute, &dns_thread_main, this) < 0)
	{
		LOGE ("DnsServer pthread_create faile!!\n");
		return false;
	}

	return true;
}

bool DnsServer::StopServer ()
{
	if (m_WorkON == false)
	{
		return false;
	}

	m_WorkON = false;

	DoDNS (NULL);

	return true;
}

bool DnsServer::UpdateDNS (char *p_address)
{
	return true;
}

bool DnsServer::DoDNS (Connection *p_target)
{
	union sigval ooo;
	int ret;
	int p_times = 300;

	ooo.sival_ptr = (void*)p_target;

again:
	ret = pthread_sigqueue (m_ThreadID, 35, ooo);
	if (ret != 0 && p_times--)
	{
		usleep (1);
		goto again;
	}

	if (p_times > 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool DnsServer::Cach (Connection *p_target)
{
	char *ip;

	ip = m_Cach.GetIpAddr (p_target->m_Domainname);

	if (ip)
	{
		SetIPv6AddrWithIPv6 (&p_target->m_ClientAddr, ip);
		p_target->m_ClientAddrOK = true;
		return true;
	}
	else
	{
		return false;
	}
}

void DnsServer::DoOne (Connection *p_target)
{
	struct addrinfo *answer, hint, *curr;

	bzero(&hint, sizeof(hint));
	hint.ai_family = AF_UNSPEC;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_protocol = 0;
	hint.ai_flags = AI_PASSIVE;

	answer = NULL;
	int ret = getaddrinfo(p_target->m_Domainname, NULL, &hint, &answer);
	if (ret != 0)
	{
		p_target->m_ClientAddrOK = false;
		freeaddrinfo(answer);
		return ;
	}
	
	m_Cach.AddDNS (p_target->m_Domainname);

	DNS_ROOT *p_dns_root;

	p_dns_root = m_Cach.GetDnsRoot (p_target->m_Domainname);

	for (curr = answer; curr != NULL; curr = curr->ai_next)
	{
		DNS_NOOD *p_dns_nood;		

		p_dns_nood = (DNS_NOOD*)malloc (sizeof(DNS_NOOD));
		p_dns_nood->next = p_dns_root->list;
		p_dns_root->list = p_dns_nood;

		if (curr->ai_family == AF_INET6)
		{

			p_target->m_ClientAddr.sin6_addr = ((struct sockaddr_in6 *)curr->ai_addr)->sin6_addr;

		}
		else if (curr->ai_family == AF_INET)
		{
			SetIPv6AddrWithIPv4 (&p_target->m_ClientAddr, (char*)&(((struct sockaddr_in *)(curr->ai_addr))->sin_addr));
		}
		memcpy (p_dns_nood->ip_addr, &p_target->m_ClientAddr.sin6_addr, 16);
		//break;
	}

	freeaddrinfo(answer);
	p_target->m_ClientAddrOK = true;
}

void DnsServer::DoWork ()
{
	struct signalfd_siginfo	p_mem[2000];	
	sigset_t sig_set;

	sigemptyset(&sig_set);
	sigaddset (&sig_set, 35);
	sigprocmask (SIG_BLOCK, &sig_set, 0);

	m_SigFd = signalfd (-1, &sig_set, 0);

	while (m_WorkON)
	{
		int ret_sum;

		ret_sum = read (m_SigFd, p_mem, sizeof(p_mem));
		if (ret_sum <= 0)
		{
			continue;
		}
		ret_sum = ret_sum / sizeof(struct signalfd_siginfo);

		for (int i=0; i < ret_sum; i++)
		{
			Connection *p_node;

			p_node = (Connection*)p_mem[i].ssi_ptr;

			if (p_node == NULL)
			{
				continue;
			}

			if (Cach (p_node))
			{
				continue;
			}
			else
			{
				DoOne (p_node);
			}
		}
	}

	close (m_SigFd);
	m_SigFd = -1;
}

