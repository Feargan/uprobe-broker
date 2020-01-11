#pragma once

#include <vector>
#include <string>

#ifdef _WIN32

#define NOMINMAX
#include <winsock2.h>
#undef NO_ERROR

#elif __linux__

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1

#else

#error unsupported platform

#endif

class CTCPServer
{
	//static_assert(std::is_base_of<CClient, TClient>::value, "template parameter must be a child of CClient");
public:
	struct CTimeValEx
	{
		timeval m_Time;
		bool m_Infinite;

		CTimeValEx(bool Infinite = true) : m_Infinite(Infinite) { m_Time = { 0, 0 }; }
		CTimeValEx(int secs, int usecs) : m_Infinite(false) { m_Time = { secs, usecs }; }

		operator timeval*()
		{
			if (m_Infinite)
				return NULL;
			return &m_Time;
		}
	};

	class CClient
	{
		SOCKET m_Socket;
		struct sockaddr_in m_Address;
		friend class CTCPServer;
	public:
		std::string getIP() const;
		unsigned int getPort() const;

		bool operator<(const CClient& r) const
		{
			return this->m_Socket < r.m_Socket;
		}

		bool operator==(const CClient& r) const
		{
			return this->m_Socket == r.m_Socket;
		}
		// add hashing function
	};
	enum
	{
		NO_ERROR = 0,

		WSASTARTUP_ERROR = 1,
		SOCKET_INIT_ERROR,
		BIND_ERROR,
		LISTEN_ERROR,
		ALREADY_RUNNING,

		CLIENT_DISCONNECT_NORMAL = 0,
		CLIENT_DISCONNECT_ERROR,
		CLIENT_DISCONNECT_TIMEOUT,
		CLIENT_DISCONNECT_KICK,

		SEND_ERROR = 1,

		RUN_ERROR = 1,
	};
private:
	bool m_Running;
	SOCKET m_Socket;
	std::vector<CClient> m_Clients;
	fd_set m_MasterFD;
	SOCKET m_MaxFD;
	CTimeValEx m_Timeout;
	
	static constexpr size_t RECV_BUF_SIZE = 128;
protected:
	virtual void onPacketRecv(const CClient& Client, const std::string& Buffer) {};
	virtual void onClientConnected(const CClient& Client) {};
	virtual void onClientDisconnected(const CClient& Client, int Reason) {};
public:
	CTCPServer() : m_Running(false) {}

	size_t numClients() { return m_Clients.size(); }
	void kickClient(CClient Client);
	void kickAll() {}						 // td

	void send(const CClient& Client, const std::string& Buffer);
	void sendAll(const std::string& Buffer);

	void setTimeout(const CTimeValEx& Timeout);

	int start(const char* IP, unsigned int Port, int MaxClients);
	int run();
	void shutdown();
private:
	// -----v  make two overloaded versions: one private - accepts const iterator, int reason, second public - accepts const CClient& and int reason
	void disconnectClient(std::vector<CClient>::const_iterator Iterator);
	void updateMaxFD();
};