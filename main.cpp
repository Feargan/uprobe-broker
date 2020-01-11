#include <iostream>
#include <ctime>

#include "BrokerServer.h"

int main()
{
	std::cout << "uProbe Remote Broker" << std::endl;
	std::cout << "Revision 0.1" << std::endl;
	std::cout << "[info] startup" << std::endl;
	CBrokerServer Server;
	Server.setTimeout(CTCPServer::CTimeValEx(0, 1000));
	switch (Server.start(nullptr, 7750, 16))
	{
	case CBrokerServer::ALREADY_RUNNING:
		std::cout << "[error] server is running already" << std::endl;
		break;
	case CBrokerServer::BIND_ERROR:
		std::cout << "[critical] failed to bind, is there any other service running on that port?" << std::endl;
		break;
	case CBrokerServer::LISTEN_ERROR:
		std::cout << "[critical] failed to listen" << std::endl;
		break;
	case CBrokerServer::WSASTARTUP_ERROR:
		std::cout << "[critical] failed to init winsock2" << std::endl;
		break;
	case CBrokerServer::SOCKET_INIT_ERROR:
		std::cout << "[critical] failed to init a socket" << std::endl;
		break;
	case CBrokerServer::NO_ERROR:
		// ok
		break;
	}

	const unsigned int autoQueryTime = 60;
	time_t LastTime;
	std::time(&LastTime);
	while (true)
	{
		Server.run();
		time_t CurrentTime;
		std::time(&CurrentTime);
		auto Elapsed = std::difftime(CurrentTime, LastTime);
		if(Elapsed >= autoQueryTime)
		{
			Server.queryAll();
			std::cout << "[info] auto query" << std::endl;
			std::time(&LastTime);
		}
		/*if (_kbhit())
		{
			char ch = _getch();
			if (ch == 'q')
			{
				cout << "[info] querying all sensors" << endl;
				Server.sendAll("QUERY");
			}
			if (ch == 'x')
			{
				cout << "[info] shutting down" << endl;
				Server.shutdown();
				break;
			}
		}*/
		//Sleep(1);
	}
	return 0;
}

