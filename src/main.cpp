#include <stdio.h>

#include "proxy_server.h"
#include "message_log.h"
#include "profile.h"

int main (int argc, char*argv[])
{
	bool ret;
	Profile     *p_Profile;
	ProxyServer *p_Sever;

	p_Profile = new Profile (argc, argv);

	LogInit (p_Profile->GetLogDir ());

	p_Profile->ShowInfor ();

	p_Sever = new ProxyServer ();

	ret = p_Sever->InitServer (p_Profile);
	if (!ret)
	{
		LOGE ("ProxyServer::InitServer Fail!!\n");
	}
	else
	{
		LOGE ("ProxyServer::Run\n");
		p_Sever->Run ();
	}

	p_Sever->CleanUp ();

	delete p_Sever;
	p_Sever = NULL;

	delete p_Profile;
	p_Profile = NULL;

	LogUninit ();
	return 0;
}
