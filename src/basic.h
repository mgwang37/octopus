#ifndef __BASIC_H__
#define __BASIC_H__

#include <netinet/in.h>
#include <netinet/tcp.h>

void SetIPv6AddrWithIPv4 (struct sockaddr_in6 *p_add, char *add);

void SetIPv6AddrWithIPv6 (struct sockaddr_in6 *p_add, char *add);

#endif /*__BASIC_H__*/

