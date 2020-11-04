#pragma once

#include "ModuleNetworking.h"

struct ChatMessage
{
	std::string username;	
	std::string message;
	bool is_whisper = false;
	std::string whispered_user;		//On a real chat scenario we probably wouldn't use the username and use a faster & more complex system, but this is just an exercise
};

class ModuleNetworkingClient : public ModuleNetworking
{
public:

	//////////////////////////////////////////////////////////////////////
	// ModuleNetworkingClient public methods
	//////////////////////////////////////////////////////////////////////

	bool start(const char *serverAddress, int serverPort, const char *playerName);

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

	void onSocketReceivedData(SOCKET socket, const InputMemoryStream& packet) override;

	void onSocketDisconnected(SOCKET socket) override;


	///////////////////////////////////////////////////////////////////////
	// Client Chat methods
	//////////////////////////////////////////////////////////////////////

	void sendChatMessage(std::string& message, bool isWhisper = false, std::string dest_username = "");



	//////////////////////////////////////////////////////////////////////
	// Client state
	//////////////////////////////////////////////////////////////////////

	enum class ClientState
	{
		Stopped,
		Start,
		Logging
	};

	ClientState state = ClientState::Stopped;

	sockaddr_in serverAddress = {};
	SOCKET socket = INVALID_SOCKET;

	std::string playerName;

	std::vector<ChatMessage> messages;
};

