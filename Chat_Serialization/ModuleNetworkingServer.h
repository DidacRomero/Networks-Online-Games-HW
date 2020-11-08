#pragma once

#include "ModuleNetworking.h"

class ModuleNetworkingServer : public ModuleNetworking
{
public:

	//////////////////////////////////////////////////////////////////////
	// ModuleNetworkingServer public methods
	//////////////////////////////////////////////////////////////////////

	bool start(int port);

	bool isRunning() const;

private:

	//////////////////////////////////////////////////////////////////////
	// Module virtual methods
	//////////////////////////////////////////////////////////////////////

	bool update() override;

	bool gui() override;



	//////////////////////////////////////////////////////////////////////
	// ModuleNetworking virtual methods
	//////////////////////////////////////////////////////////////////////

	bool isListenSocket(SOCKET socket) const override;

	void onSocketConnected(SOCKET socket, const sockaddr_in &socketAddress) override;

	void onSocketReceivedData(SOCKET socket, const InputMemoryStream& packet) override;

	void onSocketDisconnected(SOCKET socket) override;


	//////////////////////////////////////////////////////////////////////

	// User joined on true, user left on false
	void userJoinedOrLeft(std::string &username, UserConnection connection_state, std::string abort_reason = "");

	//Functions for each MESSAGE Case on RECEIVED DATA

	void onRefuseConnection(SOCKET socket, const std::string& username, std::string& reason);

	bool userIsConnected(std::string &username);

	bool muteRequest(SOCKET socket, const InputMemoryStream& packet, ServerMessage mute_or_unmute);

	bool banRequest(SOCKET socket, const InputMemoryStream& packet, ServerMessage mute_or_unmute);

	//////////////////////////////////////////////////////////////////////
	// State
	//////////////////////////////////////////////////////////////////////

	enum class ServerState
	{
		Stopped,
		Listening
	};

	ServerState state = ServerState::Stopped;

	SOCKET listenSocket;

	struct ConnectedSocket
	{
		sockaddr_in address;
		SOCKET socket;
		std::string playerName;
	};

	std::vector<ConnectedSocket> connectedSockets;
	std::vector<std::string> muted_users;
	std::vector<std::string> banned_users;
};

