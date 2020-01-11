#include <iostream>
#include <sstream>
#include <ctime>
#include <fstream>
#include <iomanip>

#include "BrokerServer.h"

#ifdef __linux__
inline static struct tm *localtime_s(struct tm *result, const time_t *timep)
{
	return localtime_r(timep, result);
}
#endif

void CBrokerServer::onClientConnected(const CClient& Client)
{
	std::cout << "[info] client connected (" << Client.getIP() << ":" << Client.getPort() << ")" << std::endl;
}

void CBrokerServer::onPacketRecv(const CClient& Client, const std::string& Buffer)
{
	std::stringstream PacketStream(Buffer);
	std::string CommandLine;
	while (std::getline(PacketStream, CommandLine))
	{
		std::stringstream CommandStream(CommandLine);
		std::string Command;
		CommandStream >> Command;
		if (Command == "REGISTER")
		{
			std::string NewSensor;
			CommandStream >> NewSensor;
			registerSensor(Client, NewSensor);
		}
		else if (Command == "DATA")
		{	
			// add some val type switch?
			std::string Property;
			float Value;
			CommandStream >> Property >> Value;
			addMeasurement(Client, Property, Value);

		}
		else if (Command == "HEARTBEAT")
		{
			std::cout << "[info] hearbeat (" << Client.getIP() << ":" << Client.getPort() << ")" << std::endl;
		}
		else
		{
			// for debugging
			//std::cout << "[info] recv (" << Client.getIP() << ":" << Client.getPort() << "): " << Buffer << std::endl;
		}
	}
}

void CBrokerServer::onClientDisconnected(const CClient& Client, int Reason)
{
	std::cout << "[info] client disconnected (" << Client.getIP() << ":" << Client.getPort() << ")" << std::endl;
	unregisterSensor(Client);
}

void CBrokerServer::registerSensor(const CClient& Client, const std::string& Name)
{
	std::cout << "[info] client requests for registration (" << Client.getIP() << ":" << Client.getPort() << ")" << std::endl;
	if (Name.empty())
	{
		std::cout << "[info] name cannot be an empty string" << std::endl;
		this->send(Client, "REGISTER INVALID\n");
		kickClient(Client);
		return;
	}
	auto It = m_Sensors.find(Client);
	if (It != m_Sensors.end())
	{
		std::cout << "[info] sensor is already registered as '" << It->second.m_Name << "'" << std::endl;
		this->send(Client, "REGISTER OK\n");
		return;
	}
	for (auto p : m_Sensors)
	{
		if (p.second.m_Name == Name)
		{
			std::cout << "[info] sensor '" << Name << "' already registered as another sensor (name conflicts/long timeout)" << std::endl;
			this->send(Client, "REGISTER CONFLICT\n");
			kickClient(Client);
			return;
		}
	}
	CSensor Sensor;
	Sensor.m_Name = Name;
	m_Sensors[Client] = Sensor;
	std::cout << "[info] sensor '" << Name << "' registered" << std::endl;
	this->send(Client, "REGISTER OK\n");
}

void CBrokerServer::unregisterSensor(const CClient& Client)
{
	auto It = m_Sensors.find(Client);
	if (It != m_Sensors.end())
	{
		std::cout << "[info] sensor '" << It->second.m_Name << "' unregistered" << std::endl;
		m_Sensors.erase(It);
	}
}

bool CBrokerServer::isRegistered(const CClient& Client)
{
	auto It = m_Sensors.find(Client);
	if (It != m_Sensors.end())
		return true;
	return false;
}

void CBrokerServer::addMeasurement(const CClient& Client, const std::string& Property, float Value)
{
	if (!isRegistered(Client))
		return;
	std::cout << "[info] data (" << m_Sensors[Client].m_Name << "@" << Client.getIP() << ":" << Client.getPort() << ") - " << Property << ": " << Value << std::endl;
	std::ofstream File(m_Sensors[Client].m_Name + "_" + Property + ".txt", std::ios_base::app);
	if (!File)
		std::cout << "[error] failed to save received data" << std::endl;
	std::time_t time = std::time(nullptr);
	std::tm date;
	localtime_s(&date, &time);
#ifdef _WIN32
	File << std::put_time(&date, "%F %H:%M:%S ") << Value << std::endl;
#else
	char Date[32];
	std::strftime(Date, 32, "%F %H:%M:%S ", &date);
	File << Date << Value << std::endl;
#endif
}

void CBrokerServer::queryAll()
{
	sendAll("QUERY\n");
}
