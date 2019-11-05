#ifndef __CLASS_DNS_SERVER__
#define __CLASS_DNS_SERVER__

#include "connection.h"
#include "dns_cache.h"

class DnsServer
{
public:
	DnsServer ();
	virtual ~DnsServer ();

	bool StartServer ();
	bool StopServer ();
	static bool DoDNS (Connection *p_target);
	static bool UpdateDNS (char *p_address);

private:
	friend void *dns_thread_main (void *p_date);
	void DoWork ();
	void DoOne (Connection *p_target);
	bool Cach (Connection *p_target);

private:
	static int        m_SigFd;
	static pthread_t  m_ThreadID;
	static bool       m_WorkON;

	DnsCache          m_Cache;
};

#endif /*__CLASS_DNS_SERVER__*/
