#ifndef __CLASS_PROFILE__
#define __CLASS_PROFILE__

#include <stdint.h>

#include <sys/socket.h>
#include <netinet/in.h>

#define ADDR_LIST_MAX        16
#define ADDR_LIST_STR_MAX    1024

class Profile 
{
public:
	Profile (int argc, char*argv[]);
	~Profile ();
	void ShowInfor ();

	int       GetListenPort ();
	int       GetThreadSum ();
	char     *GetLogDir ();
	int       GetConnectionSum ();
	uint32_t  GetHeartBeatCycle ();
	int       GetAccessMethod ();
	char     *GetUserListFile ();
	struct    sockaddr_in6 *GetAddr();

private:
	void      PrintfHelpInfo ();
	void      GetAddrList (char *optarg);

private:
	int       m_ListenPort;
	int       m_ThreadSum;
	char     *m_LogDir;
	int       m_ConnectionPerThread;
	uint32_t	 m_HeartBeatCycle;
	int       m_AccessMethod;
	char     *m_UserListFile;
	struct    sockaddr_in6 m_addr_list[ADDR_LIST_MAX];
	char      m_AddrListStr[ADDR_LIST_STR_MAX];
	int       m_AddrListSum;
	int       m_AddrListIndex;

};

#endif /*__CLASS_PROFILE__*/
