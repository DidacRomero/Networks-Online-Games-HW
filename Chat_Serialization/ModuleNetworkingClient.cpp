#include "ModuleNetworkingClient.h"


bool  ModuleNetworkingClient::start(const char * serverAddressStr, int serverPort, const char *pplayerName)
{
	playerName = pplayerName;

	bool ret = false;
	// TODO(jesus): TCP connection stuff
	// - Create the socket
	this->socket = ::socket(AF_INET, SOCK_STREAM, 0);		//Since socket() doesn't belong to a namespace we use the global namespace ( :: )

	// - Create the remote address object
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(serverPort);
	inet_pton(AF_INET, serverAddressStr, &serverAddress.sin_addr);
	
	// - Connect to the remote address
	if (connect(socket, (const struct sockaddr*)&serverAddress, sizeof(serverAddress)) == NO_ERROR)
	{
		DLOG("Connected to server SUCCESSFULLY!");
		ret = true;
	}
	else
	{
		DLOG("Error %d connecting to server", WSAGetLastError());
		closesocket(socket);
		return false;
	}


	// - Add the created socket to the managed list of sockets using addSocket()
	addSocket(socket);

	// If everything was ok... change the state
	state = ClientState::Start;
	//if (ret = true)
	//{
	//	state = ClientState::Logging;
	//}
	//else
	//{
	//	state = ClientState::Start;
	//}

	return true;
}

bool ModuleNetworkingClient::isRunning() const
{
	return state != ClientState::Stopped;
}

bool ModuleNetworkingClient::update()
{
	if (state == ClientState::Start)
	{
		// TODO: 0 (JESUS) prepare the packet
		OutputMemoryStream packet;
		packet << ClientMessage::Hello;
		packet << playerName;

		if (sendPacket(packet, socket))
		{
			state = ClientState::Logging;
		}
		else
		{
			disconnect();
			state = ClientState::Stopped;
			return true;
		}
	}

	return true;
}

bool ModuleNetworkingClient::gui()
{
	if (state != ClientState::Stopped)
	{
		// NOTE(jesus): You can put ImGui code here for debugging purposes
		ImGui::Begin("Client Window");

		Texture *tex = App->modResources->client;
		ImVec2 texSize(400.0f, 400.0f * tex->height / tex->width);
		ImGui::Image(tex->shaderResource, texSize);

		ImGui::Text("%s connected to the server...", playerName.c_str());

		ImGui::End();
	}

	return true;
}

void ModuleNetworkingClient::onSocketReceivedData(SOCKET socket, const InputMemoryStream& packet)
{
	//We will do different actions depensing on the type of message
	ServerMessage message_received;
	packet >> message_received;

	if (message_received == ServerMessage::Welcome)	//In case we are welcomed by the server (just connected)
	{
		std::string welcome_message;
		packet >> welcome_message;

		LOG("%s", welcome_message.c_str());
	}
	else if (message_received == ServerMessage::NonWelcome)
	{
		LOG("The username %s is already taken, please change your name and try again", playerName.c_str());
		state = ClientState::Stopped;
	}
	else if (message_received == ServerMessage::ChatMessage)
	{
		ChatMessage chat_message;
		packet >> chat_message.message;
		packet >> chat_message.username;

		//Add the public message to our messages list
		messages.push_back(chat_message);

		//LOG to test
		DLOG("Test Log:   %s : %s", chat_message.username.c_str(),chat_message.message.c_str());
	}
	else if (message_received == ServerMessage::Whisper)
	{
	}
	// We can use this in the future if the server sends a serverMessage::BANNED or something like this
	//state = ClientState::Stopped;
}

void ModuleNetworkingClient::onSocketDisconnected(SOCKET socket)
{
	state = ClientState::Stopped;
}


// Send a message to other users
void ModuleNetworkingClient::sendChatMessage(std::string& message, bool isWhisper)
{
	OutputMemoryStream packet;
	if (!isWhisper)
		packet << ClientMessage::ChatMessage;
	else
		packet << ClientMessage::Whisper;

	//Add infromation to the packet, WE ALWAYS WILL FOLLOW THIS FORMAT OF SERIALIZATION AND DE-SERIALIZATION
	packet << message;	//Message
	packet << playerName; //Username that sent the message
	
	if (isWhisper)
	{
		packet << isWhisper;	 // add the whisper bool
		//add the destined username to receive the message
	}

	sendPacket(packet,socket);
}

