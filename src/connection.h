#ifndef __CLASS_CONNECTION__
#define __CLASS_CONNECTION__

#include <stdint.h>

#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_LEN  2048

#include <map>
#include <string>

using namespace std;

class Connection 
{
public:
	typedef enum 
	{
	   eStepNew,
	   eStepGetMethod,
	   eStepGetAccess,
	   eStepGetProtocol,
	   eStepTcpCreat,
	   eStepTcpConnecting,
	   eStepTcpConnected,
	   eStepTcpClosing,
	   eStepClosed,
	   eStepFree,
	}WorkStep;

	typedef enum 
	{
		eMethodNull = 0x0,
		eMethodGSSAPI = 0x1,
		eMethodUSERNAME_PASSWORD = 0x2,
	}Method;

	Connection ();
	virtual ~Connection ();
	void DoWork (int type, uint32_t mask, long p_time);
	void InitNewOne (int new_socket, long p_time, int epollfd);
	static void SetSocketAttr (int socket);
	static bool LoadUserList (char *file_name);

private:
	void DoGetMethod (int type, uint32_t mask, long p_time);
	void DoGetAccess (int type, uint32_t mask, long p_time);
	void DoGetProtocol (int type, uint32_t mask, long p_time);
	void DoTcpCreat (int type, uint32_t mask, long p_time);
	void DoTcpConnecting (int type, uint32_t mask, long p_time);
	void DoTcpConnected (int type, uint32_t mask, long p_time);
	void DoTcpClosing (int type, uint32_t mask, long p_time);

	int  AnalyzeProtocolRequests (char *p_mem, int length, int &cmd);

private:
	friend class AssemblyLine;
	friend class DnsServer;
	WorkStep  m_Step;

	static Method m_Method;

	int   m_ControlSd;
	int   m_ConnectSd;

	int   m_Epollfd;

	char  m_OutBuffer[BUFFER_LEN];
	int   m_OutStart;
	int   m_OutStop;

	char  m_InBuffer[BUFFER_LEN];
	int   m_InStart;
	int   m_InStop;

	long  m_LastTime;

	bool                m_ClientAddrOK;
	struct sockaddr_in6 m_ClientAddr;
	char                m_Domainname[128];

	static map<string, string> m_UserList;

	int         m_Index;
	Connection *m_next;
};

#endif /*__CLASS_CONNECTION__*/
