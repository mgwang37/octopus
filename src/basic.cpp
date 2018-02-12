#include <strings.h>
#include <string.h>

#include "basic.h"

void SetIPv6AddrWithIPv4 (struct sockaddr_in6 *p_add, char *add)
{
	bzero (&p_add->sin6_addr.s6_addr[0], 10);
	memset (&p_add->sin6_addr.s6_addr[10], 0xff, 2);
	memcpy (&p_add->sin6_addr.s6_addr[12], add, 4);
}

void SetIPv6AddrWithIPv6 (struct sockaddr_in6 *p_add, char *add)
{
	memcpy (&p_add->sin6_addr.s6_addr[0], add, 16);
}

