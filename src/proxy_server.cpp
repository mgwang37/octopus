#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <signal.h>

#include <unistd.h>
#include <string.h>

#include "proxy_server.h"
#include "message_log.h"

static bool m_WorkON = false;

static void c_sigaction_fun (int sig)
{
	m_WorkON = false;
}

ProxyServer::ProxyServer ()
{
	m_AssemblyLines = NULL;
}

int ProxyServer::GetCpuSum ()
{
	int ret;
	char p_mem[64];
	FILE *p_sh = NULL; 
		
	p_sh = popen ("cat /proc/cpuinfo|grep processor|wc -l", "r");
	fread (p_mem, 1, 64, p_sh);
	ret = atoi (p_mem);
	pclose (p_sh);

	return ret;
}

ProxyServer::~ProxyServer ()
{
}

void ProxyServer::SignalHandlerInstall ()
{
	struct sigaction action;

	memset(&action, 0x00, sizeof(struct sigaction));

	action.sa_handler = &c_sigaction_fun;
	sigemptyset(&(action.sa_mask));
	sigaddset(&(action.sa_mask), SIGINT);
	action.sa_flags = 0;
	sigaction(SIGINT, &action, 0);
}

bool ProxyServer::InitServer (Profile *p_profile)
{
	int p_cpu_sum;

	m_ListenPort = p_profile->GetListenPort ();
	m_AssemblyLineSum = p_profile->GetThreadSum ();
	m_HeartBeatCycle = p_profile->GetHeartBeatCycle ();

	Connection::SetProfile (p_profile);

	p_cpu_sum = GetCpuSum ();

	LOGE ("CPU sum :%d\n", p_cpu_sum);

	if (m_AssemblyLineSum > p_cpu_sum)
	{
		LOGE ("pthread-sum > CPU sum !!\n");
		return false;
	}

	if (m_ListenPort ==0 || m_ListenPort > 65535)
	{
		LOGE ("m_ListenPort invalid !!\n");
		return false;
	}

	if (p_profile->GetAccessMethod () == 0x02)
	{
		if (0 == Connection::LoadUserList (p_profile->GetUserListFile()))
		{
			LOGE ("load userlist file faile!!\n");
			return false;
		}
	}

	m_AssemblyLines = new AssemblyLine[m_AssemblyLineSum];

	for (int i = 0; i < m_AssemblyLineSum; i++)
	{
		m_AssemblyLines[i].Init (m_ListenPort, p_profile->GetConnectionSum(), m_HeartBeatCycle, p_profile->GetAccessMethod ());
	}

	SignalHandlerInstall ();
	return true;
}

void ProxyServer::Run ()
{
	for (int i = 0; i < m_AssemblyLineSum; i++)
	{
		m_AssemblyLines[i].Start (i);
	}

	m_WorkON = true;

	m_DnsServer = new DnsServer ();

	m_DnsServer->StartServer ();

	while (m_WorkON)
	{
		usleep (m_HeartBeatCycle);
		AssemblyLine::HeartBeat (m_HeartBeatCycle);
	}

	m_DnsServer->StopServer ();
	delete m_DnsServer;
	m_DnsServer = NULL;

	for (int i = 0; i < m_AssemblyLineSum; i++)
	{
		m_AssemblyLines[i].Stop();
	}

	sleep (1);
}

void ProxyServer::CleanUp ()
{
	delete [] m_AssemblyLines; 
	m_AssemblyLines = NULL;
}

