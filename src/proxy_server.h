#ifndef __CLASS_PROXY_SERVER__
#define __CLASS_PROXY_SERVER__

#include "assembly_line.h"
#include "profile.h"
#include "dns_server.h"

class ProxyServer
{
public:
	ProxyServer ();
	virtual ~ProxyServer ();
	bool InitServer (Profile *p_profile);
	void Run ();
	void CleanUp ();

private:
	int  GetCpuSum ();
	void SignalHandlerInstall ();

private:
	AssemblyLine *m_AssemblyLines;
	int           m_AssemblyLineSum;
	int           m_ListenPort;
	long          m_HeartBeatCycle;

	DnsServer    *m_DnsServer;

};

#endif /*__CLASS_PROXY_SERVER__*/
