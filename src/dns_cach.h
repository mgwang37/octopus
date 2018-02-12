#ifndef __CLASS_DNS_CACH__
#define __CLASS_DNS_CACH__

#include <map>
#include <string>

using namespace std;

typedef struct DNS_NOOD_
{
	char ip_addr[16];
	struct DNS_NOOD_ *next;
}DNS_NOOD;

typedef struct DNS_ROOT_
{
	DNS_NOOD *list;
	DNS_NOOD *current;
}DNS_ROOT;

class DnsCach
{
public:
	DnsCach ();
	~DnsCach ();

	char *GetIpAddr (char *domainname);
	DNS_ROOT *GetDnsRoot (char *domainname);
	void  AddDNS (char *domainname);

private:
	void FreeNood (DNS_ROOT *p_root);

private:
	map<string, DNS_ROOT> m_MapDNS;
};

#endif /*__CLASS_DNS_CACH__*/
