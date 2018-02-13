#include <stdlib.h>
#include <arpa/inet.h>

#include "dns_cach.h"
#include "message_log.h"

DnsCach::DnsCach ()
{
}

DnsCach::~DnsCach ()
{
	map<string, DNS_ROOT>::iterator iter;

	for(iter = m_MapDNS.begin(); iter != m_MapDNS.end(); iter++)  
	{
		FreeNood (&iter->second);
	}

}

void DnsCach::FreeNood (DNS_ROOT *p_root)
{
	DNS_NOOD *p_tmp;

	p_tmp = p_root->list;

	while (p_tmp)
	{
		DNS_NOOD *pp;
		pp = p_tmp;
		free (pp);
		p_tmp = p_tmp->next;
	}

}

char *DnsCach::GetIpAddr (char *domainname)
{
	char ppp[128];

	char *ret_val = NULL;
	DNS_ROOT * p_root;

	p_root = GetDnsRoot (domainname);

	if (p_root == NULL)
	{
		return NULL;
	}
	else
	{
		if (p_root->list == NULL)
		{
			return NULL;
		}

		if (p_root->current == NULL)
		{
			p_root->current = p_root->list;
		}

		ret_val = p_root->current->ip_addr;

		p_root->current = p_root->current->next;
		inet_ntop (AF_INET6, ret_val, ppp, 128);
		LOGI ("[%s]\n", ppp);
		return ret_val;
	}

}

void DnsCach::AddDNS (char *domainname)
{
	DNS_ROOT p_tmp;

	p_tmp.list = NULL;
	p_tmp.current = NULL;

	m_MapDNS.insert(pair<string, DNS_ROOT>(domainname, p_tmp));
}

DNS_ROOT *DnsCach::GetDnsRoot (char *domainname)
{
	map<string, DNS_ROOT>::iterator iter;

	iter = m_MapDNS.find(domainname);

	if (iter != m_MapDNS.end())
	{
		return &iter->second;
	}
	else
	{
		return NULL;
	}
}

