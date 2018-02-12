#ifndef __CLASS_ASSEMBLY_LINE__
#define __CLASS_ASSEMBLY_LINE__

#include <pthread.h>
#include <stdint.h>

#include "connection.h"

class ProxyServer;

class AssemblyLine
{
public:
	AssemblyLine ();
	virtual ~AssemblyLine ();
	void Init (int Listen_port, int connect_sum, int wait_time, int method);
	void Start (int cpu_index);
	void Stop ();
	void Run ();

private:
	friend class ProxyServer;
   static void HeartBeat (int cycle);
	void BindCPU ();
	void InitConnectionPool ();
	void FreeConnectionPool ();

	Connection *AllocConnection (uint64_t &index);
	void FreeConnection (Connection *p_tmp);

	void AddToUseList (Connection *p_tmp);

	int  CreatListenSd ();
	void CloseListenSd ();

	int  CreatEpollFd ();
	void CloseEpollFd ();

	void DoRecovery ();
	void DoWork ();
	void NewConnection ();

private:
	Connection *m_ConnectionFreeList;
	Connection *m_ConnectionUseList;
	Connection *m_Connections;

	int              m_ConnectSum;
	int              m_ListenPort;
	int              m_CpuId;
	static long      m_HeartBeatTimes;
	long             m_HeartBeatTimesLast;
	pthread_t        m_ThreadID;
	bool             m_WorkON;

	int              m_ListenSd;
	int              m_EpollFd;
	int              m_EpollWaitTime;

	int              m_client_sum;
};

#endif /*__CLASS_ASSEMBLY_LINE__*/
