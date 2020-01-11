#ifdef _WIN32

#define _WIN32_WINNT _WIN32_WINNT_WIN10
#define NOMINMAX
#define _SCL_SECURE_NO_WARNINGS
#include <WS2tcpip.h>

#else

#include <sys/socket.h>
#include <arpa/inet.h>

inline static const char* InetNtop(int af, const void *src, char *dst, socklen_t size)
{
	return inet_ntop(af, src, dst, size);
}

inline static int InetPton(int af, const char *src, void *dst)
{
	return inet_pton(af, src, dst);
}

#endif

#include "TCPServer.h"

#include <algorithm>
#include <cstring>

int CTCPServer::start(const char* IP, unsigned int Port, int MaxClients)
{
	if (m_Running)
		return ALREADY_RUNNING;

#ifdef _WIN32
	WSADATA WsData;
	if (WSAStartup(MAKEWORD(2, 2), &WsData))
		return WSASTARTUP_ERROR;
#endif
	if ((m_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
		return SOCKET_INIT_ERROR;
#ifdef _WIN32
	char enable = 1;
#else
	int enable = 1;
#endif
	if (setsockopt(m_Socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		return SOCKET_INIT_ERROR;
	struct sockaddr_in Address;
	Address.sin_family = AF_INET;
	Address.sin_port = htons(Port);
	if (!IP)
		Address.sin_addr.s_addr = INADDR_ANY;
	else
		InetPton(AF_INET, IP, &(Address.sin_addr));
	std::memset(&Address.sin_zero, 0, sizeof(Address.sin_zero));
	if (bind(m_Socket, (struct sockaddr *) &Address, sizeof(Address)))
		return BIND_ERROR;
	if (listen(m_Socket, MaxClients))
		return LISTEN_ERROR;

	m_Running = true;
	m_Clients.clear();
	FD_ZERO(&m_MasterFD);
	FD_SET(m_Socket, &m_MasterFD);
	updateMaxFD();
	return NO_ERROR;
}

int CTCPServer::run()
{
	struct sockaddr_in NewAddr;

// TO PONIZEJ TO JEST SROGA WINDOWSOWSKA PARANOJA XDDD
#ifdef _WIN32
	int AddrSize = sizeof(NewAddr);
#else
	unsigned int AddrSize = sizeof(NewAddr);
#endif

	fd_set ReadFD = m_MasterFD;
	if (select(m_MaxFD+1, &ReadFD, NULL, NULL, m_Timeout) <= 0) // timed out or failed -> handle errors!!
		return NO_ERROR;

	if (FD_ISSET(m_Socket, &ReadFD)) // listener ready - incoming connection
	{
		SOCKET NewSocket = accept(m_Socket, (struct sockaddr *) &NewAddr, &AddrSize);
		if (NewSocket != SOCKET_ERROR)
		{
			CClient NewClient;
			NewClient.m_Address = NewAddr;
			NewClient.m_Socket = NewSocket;
			m_Clients.push_back(NewClient);
			FD_SET(NewSocket, &m_MasterFD);
			updateMaxFD();
			onClientConnected(NewClient);
		}
	}
	for (unsigned int i = 0; i<m_Clients.size(); i++)
	{
		if (!FD_ISSET(m_Clients[i].m_Socket, &ReadFD))
			continue;

		char Buffer[RECV_BUF_SIZE];
		int Bytes = recv(m_Clients[i].m_Socket, Buffer, RECV_BUF_SIZE - 1, 0);

		if (Bytes == -1 || Bytes == 0)
		{
			switch (Bytes)
			{
			case -1:
				onClientDisconnected(m_Clients[i], CLIENT_DISCONNECT_ERROR);
				break;
			case 0:
				onClientDisconnected(m_Clients[i], CLIENT_DISCONNECT_NORMAL);
				break;
			}
			disconnectClient(m_Clients.begin() + i);
			i--;
			continue;
		}

		std::string sBuf(Buffer, Bytes);
		onPacketRecv(m_Clients[i], sBuf);
	}
	return NO_ERROR;
}

void CTCPServer::shutdown()
{
#ifdef _WIN32
	closesocket(m_Socket);
	WSACleanup();
#else
	close(m_Socket);
#endif
	m_Running = false;
}

void CTCPServer::disconnectClient(std::vector<CClient>::const_iterator Iterator)
{
	//closesocket(m_Clients[i].m_Socket);
#ifdef _WIN32
	::closesocket(Iterator->m_Socket);
#else
	close(Iterator->m_Socket);
#endif
	FD_CLR(Iterator->m_Socket, &m_MasterFD);
	m_Clients.erase(Iterator);
	updateMaxFD();
}

void CTCPServer::kickClient(CClient Client)
{
	auto It = std::find(m_Clients.begin(), m_Clients.end(), Client);
	if (It != m_Clients.end())
	{
		onClientDisconnected(Client, CLIENT_DISCONNECT_KICK); // reason : kick
																//::send(Client.m_Socket, "KICK", 4, 0);
		disconnectClient(It);				// these two functions should be merged into one, leave disconnectClient(iterator, reason) -> it will call onClientDisconnected
		/*::shutdown(Client.m_Socket, SD_BOTH);
		FD_CLR(Client.m_Socket, &m_MasterFD);
		m_Clients.erase(It);*/
	}
}

void CTCPServer::sendAll(const std::string& Buffer)
{
	for (auto c : m_Clients)
	{
		this->send(c, Buffer);
	}
}

void CTCPServer::send(const CClient& Client, const std::string& Buffer)
{
	if (FD_ISSET(Client.m_Socket, &m_MasterFD))
		::send(Client.m_Socket, Buffer.c_str(), Buffer.size(), 0);
}

void CTCPServer::setTimeout(const CTimeValEx& Timeout)
{
	m_Timeout = Timeout;
}

//#include <iostream>

void CTCPServer::updateMaxFD()
{
	if (m_Clients.empty())
	{
		m_MaxFD = m_Socket;
		//std::cout << "clients=" << m_Clients.size() << ",maxfd=" << m_MaxFD << std::endl;
		return;
	}
	auto It = std::max_element(m_Clients.begin(), m_Clients.end());
	m_MaxFD = It->m_Socket;
	//std::cout << "clients=" << m_Clients.size() << ",maxfd=" << m_MaxFD << std::endl;
}

// CClient

std::string CTCPServer::CClient::getIP() const 
{
	char Buffer[16];
	InetNtop(AF_INET, &(m_Address.sin_addr), Buffer, 16);
	return std::string(Buffer);
}

unsigned int CTCPServer::CClient::getPort() const
{
	return ntohs(m_Address.sin_port);
}