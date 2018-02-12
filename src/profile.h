#ifndef __CLASS_PROFILE__
#define __CLASS_PROFILE__

#include <stdint.h>

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

private:
	void      PrintfHelpInfo ();

private:
	int       m_ListenPort;
	int       m_ThreadSum;
	char     *m_LogDir;
	int       m_ConnectionPerThread;
	uint32_t	 m_HeartBeatCycle;
	int       m_AccessMethod;
};

#endif /*__CLASS_PROFILE__*/
