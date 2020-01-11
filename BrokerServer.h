#pragma once

#include "TCPServer.h"

#include <map>

class CBrokerServer : public CTCPServer
{
	struct CSensor
	{
		std::string m_Name;
		//
	};
	std::map<CClient, CSensor> m_Sensors;
	virtual void onClientConnected(const CClient& Client) override;
	virtual void onPacketRecv(const CClient& Client, const std::string& Buffer) override;
	virtual void onClientDisconnected(const CClient& Client, int Reason) override;
	
	void registerSensor(const CClient& Client, const std::string& Name);
	void unregisterSensor(const CClient& Client);
	bool isRegistered(const CClient& Client);

	void addMeasurement(const CClient& Client, const std::string& Property, float Value);
public:
	void queryAll();
};